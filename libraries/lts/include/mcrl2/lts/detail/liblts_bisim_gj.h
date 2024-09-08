// Author(s): Jan Friso Groote
//
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file lts/detail/liblts_bisim_gj.h
///
/// \brief O(m log n)-time branching bisimulation algorithm similar to liblts_bisim_dnj.h
///        which does not use bunches, i.e., partitions of transitions. This algorithm
///        should be slightly faster, but in particular use less memory than liblts_bisim_dnj.h.
///        Otherwise the functionality is exactly the same. 

// TODO:
// Merge identifying whether there is a splitter and actual splitting (Done. No performance effect).
// Use BLC lists for the main split. 
// Maintain a co-splitter and a splitter to enable a co-split. 
// Eliminate two pointers in transition_type.

#ifndef LIBLTS_BISIM_GJ_H
#define LIBLTS_BISIM_GJ_H

// #include <forward_list>
#include <deque>
#include "mcrl2/utilities/hash_utility.h"
#include "mcrl2/lts/detail/liblts_scc.h"
#include "mcrl2/lts/detail/liblts_merge.h"

#ifndef NDEBUG
#define CHECK_COMPLEXITY_GJ // Check whether coroutines etc. satisfy the O(m log n) time complexity constraint for the concrete input.
                            // Outcomment to disable. Works only in debug mode. 
#endif

#ifdef CHECK_COMPLEXITY_GJ
  #include "mcrl2/lts/detail/check_complexity.h"
  // Using __VA_ARGS__ is not handled appropriately by MSVC. 
  #define mCRL2complexity_gj(A1, A2, A3) mCRL2complexity(A1, A2, A3)
#else
  #define mCRL2complexity_gj(...)  do{}while(0)
#endif

namespace mcrl2
{
namespace lts
{
namespace detail
{

template <class LTS_TYPE> class bisim_partitioner_gj;

namespace bisimulation_gj
{

// Forward declaration.
struct block_type;
struct transition_type;
struct transition_pointer_pair;

typedef std::size_t state_index;
typedef std::size_t transition_index;
typedef std::size_t block_index;
typedef std::size_t label_index;
typedef std::size_t constellation_index;
typedef std::vector<transition_pointer_pair>::iterator outgoing_transitions_it;
typedef std::vector<transition_pointer_pair>::reverse_iterator outgoing_transitions_reverse_it;
typedef std::vector<transition_pointer_pair>::const_iterator outgoing_transitions_const_it;

constexpr transition_index null_transition=-1;
// constexpr std::size_t null_pointer=-1;
constexpr label_index null_action=-1;
constexpr state_index null_state=-1;
constexpr block_index null_block=-1;
constexpr transition_index undefined=-1;
constexpr transition_index Rmarked=-2;

// The function clear takes care that a container frees memory when it is cleared and it is large. 
template <class CONTAINER>
void clear(CONTAINER& c)
{
  if (c.size()>1000) { c=CONTAINER(); } else { c.clear(); }
}

// Private linked list that uses less memory. 

template <class T>
struct linked_list_node;

template <class T>
struct linked_list_iterator
{
  linked_list_node<T>* m_iterator=nullptr;

  linked_list_iterator()=default;

  linked_list_iterator(linked_list_node<T>* t)
    : m_iterator(t)
  {}

  linked_list_iterator operator ++()
  {
    *this=m_iterator->next();
    return m_iterator;
  }

  linked_list_node<T>& operator *()
  {
    return *m_iterator;
  }

  linked_list_node<T>* operator ->()
  {
    return m_iterator;
  }

  bool operator !=(linked_list_iterator other) const
  {
    return m_iterator!=other.m_iterator;
  }

  bool operator ==(linked_list_iterator other) const
  {
    return m_iterator==other.m_iterator;
  }

};

template <class T>
struct linked_list_node: public T
{
  // typedef typename std::deque<linked_list_node<T> >::iterator iterator;
  typedef  linked_list_iterator<T> iterator;
  iterator m_next;
  iterator m_prev;

  linked_list_node(const T& t, iterator next, iterator prev)
   : T(t),
     m_next(next),
     m_prev(prev)
  {}

  iterator& next()
  {
    return m_next;
  }

  iterator& prev()
  {
    return m_prev;
  }
 
  T& content()
  {
    return static_cast<T&>(*this);
  }
};

template <class T>
struct global_linked_list_administration
{ 
  std::deque<linked_list_node<T> > m_content;
  linked_list_iterator<T> m_free_list=nullptr;
};

template <class T>
struct linked_list
{
  typedef linked_list_iterator<T> iterator;
  // std::deque<linked_list_node<T> > m_content;
  // iterator m_free_list=nullptr;

  global_linked_list_administration<T>& glla() { static global_linked_list_administration<T> glla; return glla; }
  iterator m_initial_node=nullptr;

  iterator begin() const
  {
    return m_initial_node;
  } 

  iterator end() const
  {
    return nullptr;
  }

  bool empty() const
  {
    return m_initial_node==nullptr;
  }

  bool check_linked_list() const
  {
    if (empty())
    {
      return true;
    }
    iterator i=m_initial_node;
    if (i->prev()!=nullptr) 
    {
      return false;
    }
    while (i->next()!=nullptr)
    {
      if (i->next()->prev()!=i)
      {
        return false;
      }
      i=i->next();
      if (i->prev()->next()!=i)
      {
        return false;
      }
    }
    return true;
  }

  // Puts a new element after the current element indicated by pos, unless
  // pos==nullptr, in which it is put in front of the list. 
  template <class... Args>
  iterator emplace_after(iterator pos, Args&&... args)
  {
    if (pos==nullptr)
    {
      return emplace_front(args...);
    }

    iterator new_position;
    if (glla().m_free_list==nullptr)
    {
      new_position=&glla().m_content.emplace_back(T(args...), pos->next(), pos);
    }
    else
    { 
      // Take an element from the free list. 
      new_position=glla().m_free_list;
      glla().m_free_list=glla().m_free_list->next();
      new_position->prev()=pos;
      new_position->next()=pos->next();;
      new_position->content()=T(args...);
    }
    if (pos->next()!=nullptr)
    {
      pos->next()->prev()=new_position;
    }
    pos->next()=new_position;

    return new_position;
  }

  template <class... Args>
  iterator emplace_front(Args&&... args)
  {
    iterator new_position;
    if (glla().m_free_list==nullptr)
    {
      new_position=&glla().m_content.emplace_back(T(args...), m_initial_node, nullptr);  // Deliver the address to new position.
    } 
    else
    {
      // Take an element from the free list. 
      new_position=glla().m_free_list;
      glla().m_free_list=glla().m_free_list->next();
      new_position->content()=T(args...);
      new_position->next()=m_initial_node;
      new_position->prev()=nullptr;                    
    }       
    if (m_initial_node!=nullptr)
    {
      m_initial_node->prev()=new_position;
    } 
    m_initial_node=new_position;

    return new_position;
  }

  void erase(iterator pos)
  {
    if (pos->next()!=nullptr)
    {
      pos->next()->prev()=pos->prev();
    }
    if (pos->prev()!=nullptr)
    {
      pos->prev()->next()=pos->next();
    }
    else
    {
      m_initial_node=pos->next();
    }
    pos->next()=glla().m_free_list;
    glla().m_free_list=pos;
#ifndef NDEBUG
    pos->prev()=nullptr;
#endif
  }
};

struct transition_pointer_pair
{
  transition_index transition;
  outgoing_transitions_it start_same_saC; // Refers to the last state with the same state, action and constellation,
                                          // unless it is the last, which refers to the first state.

  // The default initialiser does not initialize the fields of this struct. 
  transition_pointer_pair()
  {}

  transition_pointer_pair(const transition_index &t, const outgoing_transitions_it& sssaC)
   : transition(t),
     start_same_saC(sssaC)
  {}
};

struct label_count_sum_tuple
// David suggests: rename this struct to label_count_sum_pair to make the name more different from label_count_sum_triple.
{
  transition_index label_counter=0;
  transition_index not_investigated=0;

  // The default initialiser does not initialize the fields of this struct. 
  label_count_sum_tuple()
  {}
};

/* struct label_count_sum_triple: label_count_sum_tuple
{
  transition_index cumulative_label_counter=0;

  // The default initialiser does not initialize the fields of this struct. 
  label_count_sum_triple()
  {}
}; */

class todo_state_vector
{
  std::size_t m_todo_indicator=0;
  std::vector<state_index> m_vec;

  public:
    void add_todo(const state_index s)
    {
      assert(!find(s));
      m_vec.push_back(s);
    }

    // Move a state from the todo part to the definitive vector.
    state_index move_from_todo()
    {
      assert(!todo_is_empty());
      assert(m_todo_indicator<m_vec.size());
// It would be sufficient to write ``return m_vec[m_todo_indicator++]''.
      state_index result=m_vec[m_todo_indicator];
      m_todo_indicator++;
      return result;
    }

    std::size_t size() const
    {
      return m_vec.size();
    }

    std::size_t todo_is_empty() const
    {
      return m_vec.size()==m_todo_indicator;
    }

    std::size_t empty() const
    {
      return m_vec.empty();
    }

    bool find(const state_index s) const
    {
      return std::find(m_vec.begin(), m_vec.end(), s)!=m_vec.end();
    }

    std::vector<state_index>::const_iterator begin() const
    {
      return m_vec.begin();
    }

    std::vector<state_index>::const_iterator end() const
    {
      return m_vec.end();
    }

    void clear()
    {
      m_todo_indicator=0;
      bisimulation_gj::clear(m_vec);
    }

    void clear_todo()
    {
      m_todo_indicator=m_vec.size();
    }
};



// Below the four main data structures are listed.
struct state_type_gj
{
  block_index block=0;
  // std::vector<transition_index>::iterator start_incoming_inert_transitions;
  std::vector<transition>::iterator start_incoming_transitions;
  // std::vector<transition_index>::iterator start_incoming_non_inert_transitions;
  outgoing_transitions_it start_outgoing_transitions;
  std::vector<state_index>::iterator ref_states_in_blocks;
  transition_index no_of_outgoing_inert_transitions=0;
  transition_index counter=undefined; // This field is used to store local information while splitting. While set to -1 (undefined)
                                 // it is considered to be undefined.
                                 // When set to -2 (Rmarked) it is considered to be marked for being in R or R_todo. 
  #ifdef CHECK_COMPLEXITY_GJ
    /// \brief print a short state identification for debugging
    template<class LTS_TYPE>
    std::string debug_id_short(const bisim_partitioner_gj<LTS_TYPE>& partitioner) const
    {
        assert(&partitioner.m_states.front() <= this);
        assert(this <= &partitioner.m_states.back());
        return std::to_string(this - &partitioner.m_states.front());
    }

    /// \brief print a state identification for debugging
    template<class LTS_TYPE>
    std::string debug_id(const bisim_partitioner_gj<LTS_TYPE>& partitioner) const
    {
        return "state " + debug_id_short(partitioner);
    }

    mutable check_complexity::state_gj_counter_t work_counter;
  #endif
};

// The following type gives the start and end indications of the transitions for the same block, label and constellation
// in the array m_BLC_transitions.
struct BLC_indicators
{
  std::vector<transition_index>::iterator start_same_BLC;
  std::vector<transition_index>::iterator end_same_BLC;

  BLC_indicators(std::vector<transition_index>::iterator start, std::vector<transition_index>::iterator end)
   : start_same_BLC(start),
     end_same_BLC(end)
  {}

  #ifdef CHECK_COMPLEXITY_GJ
    /// \brief print a B_to_C slice identification for debugging
    /// \details This function is only available if compiled in Debug mode.
    template<class LTS_TYPE>
    std::string debug_id(const bisim_partitioner_gj<LTS_TYPE>& partitioner) const
    {
        assert(start_same_BLC < end_same_BLC);
        std::string result("BLC slice containing transition");
        if (std::distance(start_same_BLC, end_same_BLC) > 1)
            result += "s ";
        else
            result += " ";
        std::vector<transition_index>::const_iterator iter = start_same_BLC;
        result += partitioner.m_transitions[partitioner.m_BLC_transitions[*iter]].debug_id_short(partitioner);
        if (std::distance(start_same_BLC, end_same_BLC) > 4)
        {
            result += ", ";
            result += partitioner.m_transitions[partitioner.m_BLC_transitions[*std::next(iter)]].debug_id_short(partitioner);
            result += ", ...";
            iter = end_same_BLC - 3;
        }
        while (++iter != end_same_BLC)
        {
            result += ", ";
            result += partitioner.m_transitions[partitioner.m_BLC_transitions[*iter]].debug_id_short(partitioner);
        }
        return result;
    }

    mutable check_complexity::BLC_gj_counter_t work_counter;
  #endif
};

struct transition_type
{
  // The position of the transition type corresponds to m_aut.get_transitions(). 
  // std::size_t from, label, to are found in m_aut.get_transitions().
  linked_list<BLC_indicators>::iterator transitions_per_block_to_constellation;
  // std::vector<transition_index>::iterator ref_incoming_transitions;
  outgoing_transitions_it ref_outgoing_transitions;  // This refers to the position of this transition in m_outgoing_transitions. 
                                                     // During initialisation m_outgoing_transitions contains the indices of this 
                                                     // transition. After initialisation m_outgoing_transitions refers to the corresponding
                                                     // entry in m_BLC_transitions, of which the field transition contains the index
                                                     // of this transition. 
  // std::vector<transition_index>::iterator ref_BLC_list;  Access through ref_outgoing_transitions. 

  #ifdef CHECK_COMPLEXITY_GJ
    /// \brief print a short transition identification for debugging
    /// \details This function is only available if compiled in Debug mode.
    template<class LTS_TYPE>
    std::string debug_id_short(const bisim_partitioner_gj<LTS_TYPE>& partitioner) const
    {
        assert(&partitioner.m_transitions.front() <= this);
        assert(this <= &partitioner.m_transitions.back());
        const transition& t = partitioner.m_aut.get_transitions()[this - &partitioner.m_transitions.front()];
        return partitioner.m_states[t.from()].debug_id_short(partitioner) + " -" +
               pp(partitioner.m_aut.action_label(t.label())) + "-> " +
               partitioner.m_states[t.to()].debug_id_short(partitioner);
    }

    /// \brief print a transition identification for debugging
    /// \details This function is only available if compiled in Debug mode.
    template<class LTS_TYPE>
    std::string debug_id(const bisim_partitioner_gj<LTS_TYPE>& partitioner) const
    {
        return "transition " + debug_id_short(partitioner);
    }

    mutable check_complexity::trans_gj_counter_t work_counter;
#endif
};

struct block_type
{
  constellation_index constellation;
  std::vector<state_index>::iterator start_bottom_states;
  std::vector<state_index>::iterator start_non_bottom_states;
  std::vector<state_index>::iterator end_states;
  // The list below seems too expensive. Maybe a cheaper construction is possible. Certainly the size of the list is not important. 
  linked_list< BLC_indicators > block_to_constellation;

  block_type(const std::vector<state_index>::iterator beginning_of_states, constellation_index c)
    : constellation(c),
      start_bottom_states(beginning_of_states),
      start_non_bottom_states(beginning_of_states),
      end_states(beginning_of_states)
  {}

  #ifdef CHECK_COMPLEXITY_GJ
    /// \brief print a block identification for debugging
    template<class LTS_TYPE>
    inline std::string debug_id(const bisim_partitioner_gj<LTS_TYPE>& partitioner) const
    {
        assert(&partitioner.m_blocks.front() <= this);
        assert(this <= &partitioner.m_blocks.back());
        assert(partitioner.m_states_in_blocks.begin() <= start_bottom_states);
        assert(start_bottom_states <= start_non_bottom_states);
        assert(start_non_bottom_states <= end_states);
        assert(end_states <= partitioner.m_states_in_blocks.end());
        return "block [" + std::to_string(&*start_bottom_states - &partitioner.m_states_in_blocks.front()) + "," + std::to_string(&*end_states - &partitioner.m_states_in_blocks.front()) + ")"
                    " (#" + std::to_string(this - &partitioner.m_blocks.front()) + ")";
    }

    mutable check_complexity::block_gj_counter_t work_counter;
  #endif
};

struct constellation_type
{
// David suggests: group the states not only per block but also per constellation in m_states_in_blocks.
// Then, a constellation can be a contiguous area in m_states_in_blocks,
// and we do not need to store a list of block indices to describe a constellation
// (but only the index of the first and last state in m_states_in_blocks that belongs to the constellation).
// With that grouping, it is also not necessary to store constellations in a vector
// or a list; one can allocate each constellation as its own data structure
// and use a pointer to constellation_type as identifier of a constellation.
  std::forward_list<block_index> blocks;

  constellation_type(const block_index bi)
  {
    blocks.push_front(bi);
  }

  #ifndef NDEBUG
    /// \brief print a constellation identification for debugging
    template<class LTS_TYPE>
    inline std::string debug_id(const bisim_partitioner_gj<LTS_TYPE>& partitioner) const
    {
        assert(&partitioner.m_constellations.front() <= this);
        assert(this <= &partitioner.m_constellations.back());
        return "constellation " + std::to_string(this - &partitioner.m_constellations.front());
    }
  #endif
};

// The struct below facilitates to walk through a LBC_list starting from an arbitrary transition.
typedef std::vector<transition_index>::iterator LBC_list_iterator;

} // end namespace bisimulation_gj


/*=============================================================================
=                                 main class                                  =
=============================================================================*/


using namespace mcrl2::lts::detail::bisimulation_gj;

/// \class bisim_partitioner_gj
/// \brief implements the main algorithm for the branching bisimulation quotient
template <class LTS_TYPE>
class bisim_partitioner_gj
{
  protected:

    typedef std::unordered_set<state_index> set_of_states_type;
    typedef std::unordered_set<transition_index> set_of_transitions_type;
    typedef std::vector<constellation_index> set_of_constellations_type;

    typedef std::unordered_map<std::pair<label_index, constellation_index>, set_of_states_type> 
                      label_constellation_to_set_of_states_map;
    typedef std::unordered_map<std::pair<block_index, label_index>, std::size_t> block_label_to_size_t_map;

    #ifndef NDEBUG
      public: // needed for the debugging functions, e.g. debug_id().
    #endif
    /// \brief automaton that is being reduced
    LTS_TYPE& m_aut;
    
    // Generic data structures.
    std::vector<state_type_gj> m_states;
    // std::vector<transition_index> m_incoming_transitions;
    std::vector<transition_pointer_pair> m_outgoing_transitions;  // During initialisation this contains the transition index.
                                                                  // During refining this contains the index in m_BLC_transition, of which
                                                                  // the transition field contains the index of the transition. 
    std::vector<transition_type> m_transitions;
    std::vector<state_index> m_states_in_blocks;
    std::vector<block_type> m_blocks;
    std::vector<constellation_type> m_constellations;
    std::vector<transition_index> m_BLC_transitions;
  protected:
    std::vector<state_index> m_P;
    // Below are the two vectors that contain the marked and unmarked states, which 
    // are internally split in a part for states to be investigated, and a part for
    // states that belong definitively to this set. 
    todo_state_vector m_R, m_U;
    std::vector<state_index> m_U_counter_reset_vector;
    // The following variable contains all non trivial constellations.
    set_of_constellations_type m_non_trivial_constellations;


    /// \brief true iff branching (not strong) bisimulation has been requested
    const bool m_branching;
  
    /// \brief true iff divergence-preserving branching bisimulation has been
    /// requested
    /// \details Note that this field must be false if strong bisimulation has
    /// been requested.  There is no such thing as divergence-preserving strong
    /// bisimulation.
    const bool m_preserve_divergence;

#define is_inert_during_init(t) (m_branching && m_aut.is_tau(m_aut.apply_hidden_label_map((t).label())) && (!m_preserve_divergence || (t).from() != (t).to()))

#ifndef NDEBUG // This suppresses many unused variable warnings. 
    void check_transitions(const bool check_temporary_complexity_counters) 
    {
      // This routine can only be used after initialisation. 
      for(std::size_t ti=0; ti<m_transitions.size(); ++ti)
      {
        // const std::vector<transition_index>::iterator ind=m_transitions[ti].ref_BLC_list;
        const std::vector<transition_index>::iterator ind=m_BLC_transitions.begin() + m_transitions[ti].ref_outgoing_transitions->transition;
        assert(*ind==ti);

        const transition& t=m_aut.get_transitions()[ti];
        const block_index b=m_states[t.from()].block;
        
        bool found=false;
        for(BLC_indicators blc: m_blocks[b].block_to_constellation)
        {
          assert(blc.start_same_BLC!=blc.end_same_BLC);
          found=found || std::find(blc.start_same_BLC, blc.end_same_BLC, ti)!=blc.end_same_BLC;
        }
        assert(found);
        if (check_temporary_complexity_counters)
        {
          #ifdef CHECK_COMPLEXITY_GJ
            const block_index targetb = m_states[t.to()].block;
            const unsigned max_sourceB = check_complexity::log_n - check_complexity::ilog2(number_of_states_in_block(b));
            const unsigned max_targetC = check_complexity::log_n - check_complexity::ilog2(number_of_states_in_constellation(m_blocks[targetb].constellation));
            const unsigned max_targetB = check_complexity::log_n - check_complexity::ilog2(number_of_states_in_block(targetb));
            mCRL2complexity_gj(&m_transitions[ti], no_temporary_work(max_sourceB, max_targetC, max_targetB,
                  m_states[t.from()].no_of_outgoing_inert_transitions == 0), *this);
          #endif
        }
      }
    }

    bool check_data_structures(const std::string& tag, const bool initialisation=false, const bool check_temporary_complexity_counters=true)
    {
      mCRL2log(log::debug) << "Check data structures: " << tag << ".\n";
      assert(m_states.size()==m_aut.num_states());
      // assert(m_incoming_transitions.size()==m_aut.num_transitions());
      assert(m_outgoing_transitions.size()==m_aut.num_transitions());

      // Check that the elements in m_states are well formed. 
      for(state_index si=0; si< m_states.size(); si++)
      {
        const state_type_gj& s=m_states[si];

        assert(s.counter==undefined);
        assert(m_blocks[s.block].start_bottom_states<m_blocks[s.block].start_non_bottom_states);
        assert(m_blocks[s.block].start_non_bottom_states<=m_blocks[s.block].end_states);

        assert(std::find(m_blocks[s.block].start_bottom_states, m_blocks[s.block].end_states,si)!=m_blocks[s.block].end_states);

        /* for(std::vector<transition_index>::iterator it=s.start_incoming_inert_transitions;
                        it!=s.start_incoming_non_inert_transitions; ++it)
        {
          const transition& t=m_aut.get_transitions()[*it];
          assert(t.to()==si);
          assert(m_transitions[*it].ref_incoming_transitions==it);
          // Check that inert transitions come first. 
          assert(is_inert_during_init(t) && m_states[t.from()].block==m_states[t.to()].block);
        } 

        for(std::vector<transition_index>::iterator it=s.start_incoming_non_inert_transitions;
                        it!=m_incoming_transitions.end() &&
                        (si+1>=m_states.size() || it!=m_states[si+1].start_incoming_inert_transitions);
                     ++it)
        {
          const transition& t=m_aut.get_transitions()[*it];
          assert(t.to()==si);
          assert(m_transitions[*it].ref_incoming_transitions==it);
          // Check that inert transitions come first. 
          assert(!is_inert_during_init(t) || m_states[t.from()].block!=m_states[t.to()].block);
        } */

        const outgoing_transitions_it end_it1=(si+1>=m_states.size())?m_outgoing_transitions.end():m_states[si+1].start_outgoing_transitions;
        for(outgoing_transitions_it it=s.start_outgoing_transitions; it!=end_it1; ++it)
        {
          const transition& t=m_aut.get_transitions()
                                [initialisation ?it->transition :m_BLC_transitions[it->transition]];
          assert(t.from()==si);
          assert(!initialisation || m_transitions[it->transition].ref_outgoing_transitions==it);
          assert(initialisation || m_transitions[m_BLC_transitions[it->transition]].ref_outgoing_transitions==it);
          assert((it->start_same_saC>it && it+1!=m_outgoing_transitions.end() && 
                        ((it+1)->start_same_saC==it->start_same_saC || (it+1)->start_same_saC<=it)) || 
                 (it->start_same_saC<=it && (it+1==m_outgoing_transitions.end() || (it+1)->start_same_saC>it)));
          for(outgoing_transitions_it itt=it->start_same_saC->start_same_saC; itt<it->start_same_saC; ++itt)
          {
            const transition& t1=m_aut.get_transitions()
                                 [initialisation?itt->transition:m_BLC_transitions[itt->transition]];
            assert(t1.from()==si);
            assert(t.label()==t1.label());
            assert(m_blocks[m_states[t.to()].block].constellation==m_blocks[m_states[t1.to()].block].constellation);
          }
        }
        assert(*(s.ref_states_in_blocks)==si);

        // Check that for each state the outgoing transitions satisfy the following invariant.
        // First there are inert transitions. Then there are other transitions sorted per label
        // and constellation. 
        std::unordered_set<std::pair<label_index, constellation_index>> constellations_seen;
        const outgoing_transitions_it end_it2=(si+1>=m_states.size())?m_outgoing_transitions.end():m_states[si+1].start_outgoing_transitions;
        for(outgoing_transitions_it it=s.start_outgoing_transitions; it!=end_it2; ++it)
        {
          const transition& t=m_aut.get_transitions()[initialisation?it->transition:m_BLC_transitions[it->transition]];
// David thinks that the inert transitions should be separated from the non-inert tau transitions,
// because it may happen that non-inert tau transitions go to multiple different constellations.
// But for now we need to differentiate tau-selfloops in divergence-preserving bb.
          const label_index label = m_preserve_divergence && t.from() == t.to() && m_aut.is_tau(m_aut.apply_hidden_label_map(t.label())) ? m_aut.num_action_labels() : m_aut.apply_hidden_label_map(t.label());
          // Check that if the target constellation, if not new, is equal to the target constellation of the previous outgoing transition.
          if (constellations_seen.count(std::pair(label,m_blocks[m_states[t.to()].block].constellation))>0)
          {
            assert(it!=s.start_outgoing_transitions);
            const transition& old_t=m_aut.get_transitions()[initialisation?std::prev(it)->transition:m_BLC_transitions[std::prev(it)->transition]];
            const label_index old_label = m_preserve_divergence && old_t.from() == old_t.to() && m_aut.is_tau(m_aut.apply_hidden_label_map(old_t.label())) ? m_aut.num_action_labels() : m_aut.apply_hidden_label_map(old_t.label());
            assert(old_label == label);
            assert(m_blocks[m_states[old_t.to()].block].constellation==
                   m_blocks[m_states[t.to()].block].constellation);
          }
          // else if (m_branching && (!m_preserve_divergence || si != t.to()) && m_aut.is_tau(label) && m_blocks[m_states[t.to()].block].constellation==m_blocks[s.block].constellation)
          // {
          //   // inert transitions should come first
          //   assert(it==s.start_outgoing_transitions);
          // }
          constellations_seen.emplace(std::pair(label,m_blocks[m_states[t.to()].block].constellation));
        }
      }
      // Check that the elements in m_transitions are well formed. 
      if (!initialisation)
      {
        check_transitions(check_temporary_complexity_counters);
      }
      // Check that the elements in m_blocks are well formed. 
      {
        set_of_transitions_type all_transitions;
        for(block_index bi=0; bi<m_blocks.size(); ++bi)
        {
          const block_type& b=m_blocks[bi];
          const constellation_type& c=m_constellations[b.constellation];
          assert(std::find(c.blocks.begin(),c.blocks.end(),bi)!=c.blocks.end());
          assert(b.start_bottom_states<m_states_in_blocks.end());
          assert(b.start_bottom_states>=m_states_in_blocks.begin());
          assert(b.start_non_bottom_states<=m_states_in_blocks.end());
          assert(b.start_non_bottom_states>=m_states_in_blocks.begin());
          // David changed the following line to a strict < because every block should contain at least one bottom state.
          assert(b.start_bottom_states < b.start_non_bottom_states);
          assert(b.start_non_bottom_states <= b.end_states);
          assert(b.end_states <= m_states_in_blocks.end());

          #ifdef CHECK_COMPLEXITY_GJ
            unsigned max_B = check_complexity::log_n - check_complexity::ilog2(number_of_states_in_block(bi));
          #endif
          for(typename std::vector<state_index>::iterator is=b.start_bottom_states;
                   is!=b.start_non_bottom_states; ++is)
          {
            const state_type_gj& s=m_states[*is];
            assert(s.block==bi);
            assert(s.no_of_outgoing_inert_transitions==0);
            mCRL2complexity_gj(&s, no_temporary_work(max_B, true), *this);
          }
          for(typename std::vector<state_index>::iterator is=b.start_non_bottom_states;
                   is!=b.end_states; ++is)
          {
            const state_type_gj& s=m_states[*is];
            assert(s.block==bi);
            assert(s.no_of_outgoing_inert_transitions>0);
            mCRL2complexity_gj(&s, no_temporary_work(max_B, false), *this);
          }
          mCRL2complexity_gj(&b, no_temporary_work(check_complexity::log_n - check_complexity::ilog2(number_of_states_in_constellation(b.constellation)), max_B), *this);

          assert(b.block_to_constellation.check_linked_list());
          for(linked_list< BLC_indicators >::iterator ind=b.block_to_constellation.begin();
                     ind!=b.block_to_constellation.end(); ++ind)
          {
            const transition& first_transition=m_aut.get_transitions()[*(ind->start_same_BLC)];
            //for(LBC_list_iterator i(ind->start_same_BLC,m_transitions); i!=LBC_list_iterator(ind->end_same_BLC,m_transitions); ++i)
            for(std::vector<transition_index>::iterator i=ind->start_same_BLC; i!=ind->end_same_BLC; ++i)
            {
              const transition& t=m_aut.get_transitions()[*i];
              all_transitions.emplace(*i);
              assert(m_states[t.from()].block==bi);
              assert(m_blocks[m_states[t.to()].block].constellation==
                               m_blocks[m_states[first_transition.to()].block].constellation);
              assert(m_aut.apply_hidden_label_map(t.label())==m_aut.apply_hidden_label_map(first_transition.label()));
              if (is_inert_during_init(t) && m_blocks[m_states[t.to()].block].constellation==m_blocks[bi].constellation)
              {
                // The inert transitions should be in the first element of block_to_constellation:
                assert(b.block_to_constellation.begin()==ind);
              }
            }
            if (check_temporary_complexity_counters)
            {
              mCRL2complexity_gj(ind, no_temporary_work(check_complexity::log_n - check_complexity::ilog2(number_of_states_in_constellation(m_blocks[m_states[first_transition.to()].block].constellation))), *this);
            }
          }
        }
        assert(initialisation || all_transitions.size()==m_transitions.size());
        // destruct all_transitions here
      }

      // TODO Check that the elements in m_constellations are well formed.
      {
        std::unordered_set<block_index> all_blocks;
        for(constellation_index ci=0; ci<m_constellations.size(); ci++)
        {
          for(const block_index bi: m_constellations[ci].blocks)
          {
            assert(bi<m_blocks.size());
            assert(all_blocks.emplace(bi).second);  // Block is not already present. Otherwise a block occurs in two constellations.
          }
        }
        assert(all_blocks.size()==m_blocks.size());
        // destruct all_blocks here
      }

      // Check that the states in m_states_in_blocks refer to with ref_states_in_block to the right position.
      // and that a state is correctly designated as a (non-)bottom state. 
      for(typename std::vector<state_index>::const_iterator si=m_states_in_blocks.begin(); si!=m_states_in_blocks.end(); ++si)
      {
        assert(si==m_states[*si].ref_states_in_blocks);
      }
      
      // Check that the states in m_P are bottom states.
      for(const state_index si: m_P)
      {
        bool found_inert_outgoing_transition=false;
        const outgoing_transitions_it end_it=(si+1>=m_states.size())? m_outgoing_transitions.end():m_states[si+1].start_outgoing_transitions;
        for(outgoing_transitions_it it=m_states[si].start_outgoing_transitions; it!=end_it; ++it)
        {
          const transition& t=m_aut.get_transitions()[initialisation?it->transition:m_BLC_transitions[it->transition]];
          if (is_inert_during_init(t) && m_states[t.from()].block==m_states[t.to()].block)
          {
            found_inert_outgoing_transition=true;
          }
        }
        assert(!found_inert_outgoing_transition);
      }

      // Check that the non-trivial constellations are non trivial.
      for(const constellation_index ci: m_non_trivial_constellations)
      {
        // There are at least two blocks in a non-trivial constellation.
        std::forward_list<block_index>::const_iterator blocks_it=m_constellations[ci].blocks.begin();
        assert(blocks_it!=m_constellations[ci].blocks.end());
        blocks_it++;
        assert(blocks_it!=m_constellations[ci].blocks.end());
      }
      return true;
    }

    bool check_stability(const std::string& tag)
    {
      // Checks the following invariant:
      //     If a block has a non-inert transition, then every bottom state has a non-inert transition
      //     with the same label to the same target constellation.
      // It is assumed that the BLC data structure is correct, so we conveniently
      // use that to verify the invariant.
      mCRL2log(log::debug) << "Check stability: " << tag << ".\n";
      for(block_index bi=0; bi<m_blocks.size(); ++bi)
      {
        const block_type& b=m_blocks[bi];
        for(linked_list< BLC_indicators >::iterator ind=b.block_to_constellation.begin();
                     ind!=b.block_to_constellation.end(); ++ind)
        {
          set_of_states_type all_source_bottom_states;
          bool all_transitions_in_BLC_are_inert = true;
          for(std::vector<transition_index>::iterator i=ind->start_same_BLC; i!=ind->end_same_BLC; ++i)
          {
            const transition& t=m_aut.get_transitions()[*i];
            if (!(is_inert_during_init(t) && m_blocks[m_states[t.to()].block].constellation==m_blocks[bi].constellation))
            {
              // This is a constellation-non-inert transition.
              all_transitions_in_BLC_are_inert = false;
              if (0 == m_states[t.from()].no_of_outgoing_inert_transitions)
              {
                all_source_bottom_states.emplace(t.from());
              }
            }
          }
          // check that every bottom state has a transition in this BLC entry:
          if (!all_transitions_in_BLC_are_inert)
          {
            assert(static_cast<std::ptrdiff_t>(all_source_bottom_states.size())==std::distance(m_blocks[bi].start_bottom_states, m_blocks[bi].start_non_bottom_states));
          }
        }
      }
      return true;
    }
#endif //#ifndef NDEBUG

  #ifdef CHECK_COMPLEXITY_GJ
    // assign work to all transitions from state si with label a to constellation C.
    // It is ensured that there will be some work assigned.
    void add_work_to_same_saC(const bool initialisation, const state_index si, const label_index a, const constellation_index C, const enum check_complexity::counter_type ctr, const unsigned max_value)
    {
      assert(DONT_COUNT_TEMPORARY != max_value);
      if (m_aut.num_action_labels() == a)
      {
        assert(m_preserve_divergence);
        assert(C == m_blocks[m_states[si].block].constellation);
      }
      else
      {
        assert(a < m_aut.num_action_labels());
        assert(m_aut.apply_hidden_label_map(a) == a);
      }
      bool work_assigned = false;
      for (outgoing_transitions_it outtrans = (si + 1 >= m_states.size() ? m_outgoing_transitions.end() : m_states[si +  1].start_outgoing_transitions);
                  outtrans != m_states[si].start_outgoing_transitions; )
      {
        --outtrans;
        const transition& t = m_aut.get_transitions()[initialisation?outtrans->transition:m_BLC_transitions[outtrans->transition]];
        if (m_aut.num_action_labels() == a
            ? t.from() == t.to() && m_aut.is_tau(m_aut.apply_hidden_label_map(t.label()))
            : m_aut.apply_hidden_label_map(t.label()) == a && m_blocks[m_states[t.to()].block].constellation == C)
        {
          mCRL2complexity_gj(&m_transitions[initialisation?outtrans->transition:m_BLC_transitions[outtrans->transition]], add_work(ctr, max_value), *this);
          work_assigned = true;
        }
      }
      if (!work_assigned)
      {
        mCRL2log(log::error) << "No suitable transition " << m_states[si].debug_id_short(*this)
            << " -" << pp(m_aut.action_label(m_aut.num_action_labels() == a ? m_aut.tau_label_index() : a)) << "-> "
            << (m_aut.num_action_labels() == a ? m_states[si].debug_id_short(*this) : m_constellations[C].debug_id(*this))
            << " found to assign work for counter \""
                << check_complexity::work_names[ctr - check_complexity::BLOCK_MIN] << "\"\n";
        exit(EXIT_FAILURE);
      }
    }
  #else
    #define add_work_to_same_saC(initialisation, si, a, C, ctr, max_value) do{}while (0)
  #endif //#ifdef CHECK_COMPLEXITY_GJ

    void display_BLC_list(const block_index bi) const
    {
      mCRL2log(log::debug) << "\n  BLC_List\n";
      for(const BLC_indicators blc_it: m_blocks[bi].block_to_constellation)
      {
        mCRL2log(log::debug) << "\n    BLC_sublist:  " << std::distance(m_BLC_transitions.begin(),static_cast<std::vector<transition_index>::const_iterator>(blc_it.start_same_BLC)) << " -- "
                             << std::distance(m_BLC_transitions.begin(),static_cast<std::vector<transition_index>::const_iterator>(blc_it.end_same_BLC)) << "\n";
        for(std::vector<transition_index>::iterator i=blc_it.start_same_BLC; i!=blc_it.end_same_BLC; ++i)
        {
           const transition& t=m_aut.get_transitions()[*i];
           mCRL2log(log::debug) << "        " << t.from() << " -" << m_aut.action_label(t.label()) << "-> " << t.to() << "\n";
        }
      }
      mCRL2log(log::debug) << "  BLC_List end\n";
    }


    void print_data_structures(const std::string& header, const bool initialisation=false) const
    {
      if (!mCRL2logEnabled(log::debug))  return;
      mCRL2log(log::debug) << "========= PRINT DATASTRUCTURE: " << header << " =======================================\n";
      mCRL2log(log::debug) << "++++++++++++++++++++  States     ++++++++++++++++++++++++++++\n";
      for(state_index si=0; si<m_states.size(); ++si)
      {
        mCRL2log(log::debug) << "State " << si <<" (Block: " << m_states[si].block <<"):\n";
        mCRL2log(log::debug) << "  #Inert outgoing transitions " << m_states[si].no_of_outgoing_inert_transitions <<"):\n";

        mCRL2log(log::debug) << "  Incoming transitions:\n";
        std::vector<transition>::iterator end=(si+1==m_states.size()?m_aut.get_transitions().end():m_states[si+1].start_incoming_transitions);
        for(std::vector<transition>::iterator it=m_states[si].start_incoming_transitions; it!=end; ++it)
        {
           mCRL2log(log::debug) << "  " << ptr(*it) << "\n";
        }

        /* mCRL2log(log::debug) << "  Incoming non-inert transitions:\n";
        for(std::vector<transition_index>::iterator it=m_states[si].start_incoming_non_inert_transitions;
                        it!=m_incoming_transitions.end() &&
                        (si+1>=m_states.size() || it!=m_states[si+1].start_incoming_inert_transitions);
                     ++it)
        {
           const transition& t=m_aut.get_transitions()[*it];
           mCRL2log(log::debug) << "  " << t.from() << " -" << m_aut.action_label(t.label()) << "-> " << t.to() << "\n";
        } */

        /* mCRL2log(log::debug) << "  Incoming non-inert transitions:\n";
        for(std::vector<transition>::iterator it=m_states[si].start_incoming_transitions;
                        it!=m_aut.get_transitions().end() &&
                        (si+1>=m_aut.get_transitions().size() || it!=m_states[si+1].start_incoming_transitions);
                     ++it)
        {
           // const transition& t=m_aut.get_transitions()[*it];
           mCRL2log(log::debug) << ptr(*it) << "\n";
        } */

        mCRL2log(log::debug) << "  Outgoing transitions:\n";
        for(outgoing_transitions_it it=m_states[si].start_outgoing_transitions;
                        it!=m_outgoing_transitions.end() &&
                        (si+1>=m_states.size() || it!=m_states[si+1].start_outgoing_transitions);
                     ++it)
        {
           const transition& t=m_aut.get_transitions()[initialisation?it->transition:m_BLC_transitions[it->transition]];
           mCRL2log(log::debug) << "  " << t.from() << " -" << m_aut.action_label(t.label()) << "-> " << t.to() << "\n";;
        }
        mCRL2log(log::debug) << "  Ref states in blocks: " << *(m_states[si].ref_states_in_blocks) << ". Must be " << si <<".\n";
        mCRL2log(log::debug) << "---------------------------------------------------\n";
      }
      mCRL2log(log::debug) << "++++++++++++++++++++ Transitions ++++++++++++++++++++++++++++\n";
      for(state_index ti=0; ti<m_transitions.size(); ++ti)
      {
        const transition& t=m_aut.get_transitions()[ti];
        mCRL2log(log::debug) << "Transition " << ti <<": " << t.from() 
                                              << " -" << m_aut.action_label(t.label()) << "-> " 
                                              << t.to() << "\n";
      }

      mCRL2log(log::debug) << "++++++++++++++++++++ Blocks ++++++++++++++++++++++++++++\n";
      for(state_index bi=0; bi<m_blocks.size(); ++bi)
      {
        mCRL2log(log::debug) << "  Block " << bi << " (const: " << m_blocks[bi].constellation <<"):\n";
        mCRL2log(log::debug) << "  Bottom states: ";
        for(typename std::vector<state_index>::iterator sit=m_blocks[bi].start_bottom_states; 
                        sit!=m_blocks[bi].start_non_bottom_states; ++sit)
        {
          mCRL2log(log::debug) << *sit << "  ";
        }
        mCRL2log(log::debug) << "\n  Non bottom states: ";
        for(typename std::vector<state_index>::iterator sit=m_blocks[bi].start_non_bottom_states; 
                                 sit!=m_blocks[bi].end_states; ++sit)
        {
          mCRL2log(log::debug) << *sit << "  ";
        }
        if (!initialisation)
        {
          display_BLC_list(bi);
        }
        mCRL2log(log::debug) << "\n";
      }

      mCRL2log(log::debug) << "++++++++++++++++++++ Constellations ++++++++++++++++++++++++++++\n";
      for(state_index ci=0; ci<m_constellations.size(); ++ci)
      {
        mCRL2log(log::debug) << "  Constellation " << ci << ":\n";
        mCRL2log(log::debug) << "    Blocks in constellation: ";
        for(const block_index bi: m_constellations[ci].blocks)
        {
           mCRL2log(log::debug) << bi << " ";
        }
        mCRL2log(log::debug) << "\n";
      }
      mCRL2log(log::debug) << "Non trivial constellations: ";
      for(const constellation_index ci: m_non_trivial_constellations)
      {
        mCRL2log(log::debug) << ci << " ";
      }

      mCRL2log(log::debug) << "\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
      /* mCRL2log(log::debug) << "Incoming transitions:\n";
      for(const transition_index ti: m_incoming_transitions)
      {
        const transition& t=m_aut.get_transitions()[ti];
        mCRL2log(log::debug) << "  " << t.from() << " -" << m_aut.action_label(t.label()) << "-> " << t.to() << "\n";
      } */
        
      mCRL2log(log::debug) << "Outgoing transitions:\n";

      for(const transition_pointer_pair pi: m_outgoing_transitions)
      {
        const transition& t=m_aut.get_transitions()[initialisation?pi.transition:m_BLC_transitions[pi.transition]];
        mCRL2log(log::debug) << "  " << t.from() << " -" << m_aut.action_label(t.label()) << "-> " << t.to();
        const transition& t1=m_aut.get_transitions()[initialisation?pi.start_same_saC->transition:m_BLC_transitions[pi.start_same_saC->transition]]; 
        mCRL2log(log::debug) << "  \t(same saC: " << t1.from() << " -" << m_aut.action_label(t1.label()) << "-> " << t1.to() << ");\n";
      }

      mCRL2log(log::debug) << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
      mCRL2log(log::debug) << "New bottom states to be investigated: ";
      
      for(state_index si: m_P)
      {
        mCRL2log(log::debug) << si << " ";
      }
        
      mCRL2log(log::debug) << "\n========= END PRINT DATASTRUCTURE: " << header << " =======================================\n";
    }

  public:
    /// \brief constructor
    /// \details The constructor constructs the data structures and immediately
    /// calculates the partition corresponding with the bisimulation quotient.
    /// It destroys the transitions on the LTS (to save memory) but does not
    /// adapt the LTS to represent the quotient's transitions.
    /// It is assumed that there are no tau-loops in aut.
    /// \param aut                 LTS that needs to be reduced
    /// \param branching           If true branching bisimulation is used,
    ///                                otherwise strong bisimulation is
    ///                                applied.
    /// \param preserve_divergence If true and branching is true, preserve
    ///                                tau loops on states.
    bisim_partitioner_gj(LTS_TYPE& aut, 
                         const bool branching = false,
                         const bool preserve_divergence = false)
      : m_aut(aut),
        m_states(aut.num_states()),
        m_transitions(aut.num_transitions()),
        m_blocks(1,{m_states_in_blocks.begin(),0}),
        m_constellations(1,constellation_type(0)),   // Algorithm 1, line 1.2.
        m_BLC_transitions(aut.num_transitions()),
        m_branching(branching),
        m_preserve_divergence(preserve_divergence)
    {
      assert(m_branching || !m_preserve_divergence);
      mCRL2log(log::verbose) << "Start initialisation.\n";
      create_initial_partition();        
      mCRL2log(log::verbose) << "After initialisation there are " << m_blocks.size() << " equivalence classes. Start refining. \n";
      refine_partition_until_it_becomes_stable();
      assert(check_data_structures("READY"));
    }


    /// \brief Calculate the number of equivalence classes
    /// \details The number of equivalence classes (which is valid after the
    /// partition has been constructed) is equal to the number of states in the
    /// bisimulation quotient.
    std::size_t num_eq_classes() const
    {
      return m_blocks.size();
    }


    /// \brief Get the equivalence class of a state
    /// \details After running the minimisation algorithm, this function
    /// produces the number of the equivalence class of a state.  This number
    /// is the same as the number of the state in the minimised LTS to which
    /// the original state is mapped.
    /// \param s state whose equivalence class needs to be found
    /// \returns sequence number of the equivalence class of state s
    state_index get_eq_class(const state_index si) const
    {
      assert(si<m_states.size());
      return m_states[si].block;
    }


    /// \brief Adapt the LTS after minimisation
    /// \details After the efficient branching bisimulation minimisation, the
    /// information about the quotient LTS is only stored in the partition data
    /// structure of the partitioner object.  This function exports the
    /// information back to the LTS by adapting its states and transitions:  it
    /// updates the number of states and adds those transitions that are
    /// mandated by the partition data structure.  If desired, it also creates
    /// a vector containing an arbritrary (example) original state per
    /// equivalence class.
    ///
    /// The main parameter and return value are implicit with this function: a
    /// reference to the LTS was stored in the object by the constructor.
    void finalize_minimized_LTS()
    {
      // The transitions are most efficiently directly extracted from the block_to_constellation lists in blocks. 
      std::vector<transition> T;
      for(block_index bi=0; bi<m_blocks.size(); bi++) 
      {
        const block_type& B=m_blocks[bi];
        for(const BLC_indicators blc_ind: B.block_to_constellation)
        {
          mCRL2complexity_gj(&blc_ind, add_work(check_complexity::finalize_minimized_LTS__handle_transition, 1), *this);
          const transition& t= m_aut.get_transitions()[*blc_ind.start_same_BLC];
          const transition_index new_to=get_eq_class(t.to());
          if (!is_inert_during_init(t) || bi!=new_to)
          {
            T.emplace_back(bi, t.label(), new_to);
          }
        }
      }
      m_aut.clear_transitions();
      for (const transition& t: T)
      {
        m_aut.add_transition(t);
      } 
      //
      // Merge the states, by setting the state labels of each state to the
      // concatenation of the state labels of its equivalence class.

      if (m_aut.has_state_info())   /* If there are no state labels this step is not needed */
      {
        /* Create a vector for the new labels */
        std::vector<typename LTS_TYPE::state_label_t> new_labels(num_eq_classes());

    
        for(std::size_t i=0; i<m_aut.num_states(); ++i)
        {
          mCRL2complexity_gj(&m_states[i], add_work(check_complexity::finalize_minimized_LTS__collect_labels_of_state, 1), *this);
          const state_index new_index(get_eq_class(i));
          new_labels[new_index]=new_labels[new_index]+m_aut.state_label(i);
        }

        m_aut.set_num_states(num_eq_classes());
        for (std::size_t i=0; i<num_eq_classes(); ++i)
        {
          mCRL2complexity_gj(&m_blocks[i], add_work(check_complexity::finalize_minimized_LTS__set_labels_of_block, 1), *this);
          m_aut.set_state_label(i, new_labels[i]);
        }
      }
      else
      {
        m_aut.set_num_states(num_eq_classes());
      }

      m_aut.set_initial_state(get_eq_class(m_aut.initial_state()));
    }


    /// \brief Check whether two states are in the same equivalence class.
    /// \param s first state that needs to be compared.
    /// \param t second state that needs to be compared.
    /// \returns true iff the two states are in the same equivalence class.
    bool in_same_class(state_index const s, state_index const t) const
    {
      return get_eq_class(s) == get_eq_class(t);
    }
  protected:

    std::string ptr(const transition& t) const
    {
      return std::to_string(t.from()) + " -" + pp(m_aut.action_label(t.label())) + "-> " + std::to_string(t.to());
    }

    std::string ptr(const transition_index ti) const
    {
      const transition& t=m_aut.get_transitions()[ti];
      return ptr(t);
    }

    /*--------------------------- main algorithm ----------------------------*/

    /*----------------- splitB -- Algorithm 3 of [GJ 2024] -----------------*/

    state_index number_of_states_in_block(const block_index B) const
    {
      return std::distance(m_blocks[B].start_bottom_states, m_blocks[B].end_states);
    }

    state_index number_of_states_in_constellation(const constellation_index C) const
    {
      state_index result = 0;
      for (const block_index bi: m_constellations[C].blocks)
      {
        result += number_of_states_in_block(bi);
      }
      return result;
    }

    void swap_states_in_states_in_block(
              typename std::vector<state_index>::iterator pos1, 
              typename std::vector<state_index>::iterator pos2) 
    {
      assert(pos1!=pos2);
      std::swap(*pos1,*pos2);
      m_states[*pos1].ref_states_in_blocks=pos1;
      m_states[*pos2].ref_states_in_blocks=pos2;
    }

    // Move pos1 to pos2, pos2 to pos3 and pos3 to pos1;
    void swap_states_in_states_in_block(
              typename std::vector<state_index>::iterator pos1, 
              typename std::vector<state_index>::iterator pos2, 
              typename std::vector<state_index>::iterator pos3) 
    {
// David suggests: actually it is enough to require
// assert(pos1 != pos2 || pos2 == pos3);
// (Memory help: pos3 should be between pos1 and pos2.)
      assert(pos1!=pos2 && pos2!=pos3 && pos3!=pos1);
      const state_index temp=*pos1;
      *pos1=*pos3;
      *pos3=*pos2;
      *pos2=temp;

      m_states[*pos1].ref_states_in_blocks=pos1;
      m_states[*pos2].ref_states_in_blocks=pos2;
      m_states[*pos3].ref_states_in_blocks=pos3;
    }

    // Split the block B by moving the elements in R to the front in m_states, and add a 
    // new element B_new at the end of m_blocks referring to R. Adapt B.start_bottom_states,
    // B.start_non_bottom_states and B.end_states, and do the same for B_new. 
    block_index split_block_B_into_R_and_BminR(
                     const block_index B, 
                     const todo_state_vector& R, 
                     std::function<void(const state_index)> update_Ptilde)
    {
//std::cerr << "SPLIT BLOCK " << B << " by removing "; for(auto s:R){ std::cerr << s << " "; } std::cerr << "\n";
      // Basic administration. Make a new block and add it to the current constellation.
      const block_index B_new=m_blocks.size();
      m_blocks.emplace_back(m_blocks[B].start_bottom_states,m_blocks[B].constellation);
      #ifdef CHECK_COMPLEXITY_GJ
        m_blocks[B_new].work_counter = m_blocks[B].work_counter;
      #endif
      // m_non_trivial_constellations.emplace(m_blocks[B].constellation);
      std::forward_list<block_index>::const_iterator cit=m_constellations[m_blocks[B].constellation].blocks.begin();
      assert(cit!=m_constellations[m_blocks[B].constellation].blocks.end());
      cit++;
      if (cit==m_constellations[m_blocks[B].constellation].blocks.end()) // This constellation is trivial.
      {
        // This constellation is trivial, as it will be split add to the non trivial constellations. 
        assert(std::find(m_non_trivial_constellations.begin(),m_non_trivial_constellations.end(),m_blocks[B].constellation)==m_non_trivial_constellations.end());
        m_non_trivial_constellations.emplace_back(m_blocks[B].constellation);
      }
      m_constellations[m_blocks[B].constellation].blocks.push_front(B_new);

      // Carry out the split. 
      #ifdef CHECK_COMPLEXITY_GJ
        // The size of the new block is not yet fixed.
        const state_index new_block_size = R.size();
      #endif
      for(state_index s: R)
      {
        mCRL2complexity_gj(&m_states[s], add_work(check_complexity::split_block_B_into_R_and_BminR__carry_out_split,
                check_complexity::log_n - check_complexity::ilog2(new_block_size)), *this);
//std::cerr << "MOVE STATE TO NEW BLOCK: " << s << "\n";
        m_states[s].block=B_new;
        update_Ptilde(s);
        typename std::vector<state_index>::iterator pos=m_states[s].ref_states_in_blocks;
        if (pos>=m_blocks[B].start_non_bottom_states) // the state is a non bottom state.
        {
// David suggests: change the assertion in swap_state_in_states_in_block() so you do not need to distinguish cases here.
// Doing additional assignments is likely faster than all these branches.
// (perhaps only distinguish between no swap is needed at all/some swap is needed)
// JFG: I am not so sure whether swapping more is better, as it requires more memory accesses, possibly outside the cache.
//      But we could attempt to clean up this code. 
          // We know that B must have a bottom state. Not true all bottom states can be removed from B. 
          // assert(m_blocks[B].start_bottom_states!=m_blocks[B].start_non_bottom_states);
          if (pos==m_blocks[B].start_bottom_states)
          {
            // There is no bottom state left. pos is a non bottom state. No swap is necessary as it is
            // already at the right position.
          }
          else
          {
            if (pos==m_blocks[B].start_non_bottom_states)
            {
              // pos is not also located at the first bottom state, so a swap is needed.
              // Otherwise no swap is required. 
              swap_states_in_states_in_block(pos,m_blocks[B].start_bottom_states);
            }
            else
            {
              // pos is a later non-bottom-state of B. We need to swap:
              // pos --> B.start_bottom_states --> B.start_non_bottom_states --> pos.
              if (m_blocks[B].start_bottom_states==m_blocks[B].start_non_bottom_states)
              {
                swap_states_in_states_in_block(pos,m_blocks[B].start_bottom_states);
              }
              else
              {
                swap_states_in_states_in_block(pos,m_blocks[B].start_bottom_states, m_blocks[B].start_non_bottom_states);
              }
            }
          }
          m_blocks[B].start_non_bottom_states++;
          m_blocks[B].start_bottom_states++;
          m_blocks[B_new].end_states++;
        }
        else // the state is a bottom state
        {
// David suggests: change the assertion in swap_state_in_states_in_block() so you do not need to distinguish cases here.
// Doing additional assignments is likely faster than all these branches.
// (perhaps only distinguish between no swap is needed at all/some swap is needed)
// Also, think whether R always starts with bottom states and then continues with nonbottom states;
// in that case, the iteration over R could be split into two parts, the first part being over bottom states
// and the second part over nonbottom states. The first part then can assume that B_new does not (yet) contain nonbottom states.
          // Three cases. pos==B_new.start_non_bottom_states
          if (pos==m_blocks[B_new].start_non_bottom_states)
          {
            // It holds that B.start_bottom_states==B_new_bottom_states.
            // Nothing needs to be swapped. 
          }
          else if (m_blocks[B_new].start_non_bottom_states==m_blocks[B].start_bottom_states)
          {
            // There are no non-bottom states in B_new. 
            swap_states_in_states_in_block(pos,m_blocks[B].start_bottom_states);
          }
          else if (m_blocks[B_new].start_non_bottom_states==m_blocks[B].start_bottom_states)
          {
            // pos --> B.start_bottom_states --> pos.
            swap_states_in_states_in_block(pos,m_blocks[B].start_bottom_states);
          }
          else if (pos==m_blocks[B].start_bottom_states)
          {
            // pos --> new_B.start_non_bottom_states --> pos.
            swap_states_in_states_in_block(pos,m_blocks[B_new].start_non_bottom_states);
          }
          else
          {
            // pos --> B_new.start_non_bottom_states --> B.start_bottom_states --> pos.
            swap_states_in_states_in_block(pos,m_blocks[B_new].start_non_bottom_states,m_blocks[B].start_bottom_states);
          }
          m_blocks[B_new].start_non_bottom_states++;
          m_blocks[B_new].end_states++;
          m_blocks[B].start_bottom_states++;
          assert(m_blocks[B].start_bottom_states<=m_blocks[B].start_non_bottom_states);
          assert(m_blocks[B_new].start_bottom_states<m_blocks[B_new].start_non_bottom_states);
        }
      }
      return B_new;
    }
    
    // It is assumed that the new block is located precisely before the old_block in m_BLC_transitions.
    // This routine can not be used in the initialisation phase. It can only be used during refinement. 
    bool swap_in_the_doubly_linked_list_LBC_in_blocks(
               const transition_index ti,
               linked_list<BLC_indicators>::iterator new_BLC_block,
               linked_list<BLC_indicators>::iterator old_BLC_block)
    {
      assert(new_BLC_block->end_same_BLC==old_BLC_block->start_same_BLC);
      // std::vector<transition_index>::iterator old_position=m_transitions[ti].ref_BLC_list;
      std::vector<transition_index>::iterator old_position=m_BLC_transitions.begin()+m_transitions[ti].ref_outgoing_transitions->transition;
      std::vector<transition_index>::iterator new_position=new_BLC_block->end_same_BLC;
      assert(new_position <= old_position);
      if (old_position!=new_position)
      {
        std::swap(*old_position,*new_position);
        // m_transitions[*old_position].ref_BLC_list=old_position;
        // m_transitions[*new_position].ref_BLC_list=new_position;
        m_transitions[*old_position].ref_outgoing_transitions->transition=std::distance(m_BLC_transitions.begin(),old_position);
        m_transitions[*new_position].ref_outgoing_transitions->transition=std::distance(m_BLC_transitions.begin(),new_position);
      }
      m_transitions[ti].transitions_per_block_to_constellation=new_BLC_block;
      new_BLC_block->end_same_BLC++;
      old_BLC_block->start_same_BLC++;
      if (old_BLC_block->start_same_BLC==old_BLC_block->end_same_BLC)
      {
        return true; // last element from the old block is removed;
      }
      return false;
    }

    // Move the transition t with transition index ti to a new 
    // LBC list as the target state switches to a new constellation.
    void update_the_doubly_linked_list_LBC_new_constellation(
               const block_index index_block_B, 
               const transition& t,
               const transition_index ti)
    {
      assert(m_states[t.to()].block==index_block_B);
      assert(&m_aut.get_transitions()[ti] == &t);
      bool last_element_removed;
      linked_list<BLC_indicators>::iterator this_block_to_constellation=
                           m_transitions[ti].transitions_per_block_to_constellation;
      assert(this_block_to_constellation!= m_blocks[m_states[t.from()].block].block_to_constellation.end());
      // if this transition is inert, it is inserted in a block in front. Otherwise, it is inserted after
      // the current element in the list. 
      if (is_inert_during_init(t) && m_states[t.from()].block==index_block_B)
      {
        linked_list<BLC_indicators>::iterator first_block_to_constellation=m_blocks[m_states[t.from()].block].block_to_constellation.begin();
        assert(first_block_to_constellation->start_same_BLC != first_block_to_constellation->end_same_BLC);
        assert(m_states[m_aut.get_transitions()[*(first_block_to_constellation->start_same_BLC)].from()].block==index_block_B);
        assert(m_aut.is_tau(m_aut.apply_hidden_label_map(m_aut.get_transitions()[*(first_block_to_constellation->start_same_BLC)].label())));
        if (first_block_to_constellation==this_block_to_constellation)
        { 
          // Make a new entry in the list block_to_constellation, at the beginning;
          
          first_block_to_constellation=
                  m_blocks[m_states[t.from()].block].block_to_constellation.
                           emplace_front(//first_block_to_constellation, 
                                   this_block_to_constellation->start_same_BLC, 
                                   this_block_to_constellation->start_same_BLC);
          #ifdef CHECK_COMPLEXITY_GJ
            first_block_to_constellation->work_counter = this_block_to_constellation->work_counter;
          #endif
        }
        else  assert(m_states[m_aut.get_transitions()[*(first_block_to_constellation->start_same_BLC)].to()].block==index_block_B);
        last_element_removed=swap_in_the_doubly_linked_list_LBC_in_blocks(ti,  first_block_to_constellation, this_block_to_constellation);
      }
      else 
      {
        // std::list<BLC_indicators>::iterator next_block_to_constellation=
        //                     ++std::list<BLC_indicators>::iterator(this_block_to_constellation);
        linked_list<BLC_indicators>::iterator next_block_to_constellation=this_block_to_constellation;
        ++next_block_to_constellation;
        if (next_block_to_constellation==m_blocks[m_states[t.from()].block].block_to_constellation.end() ||
            (assert(m_states[m_aut.get_transitions()[*(next_block_to_constellation->start_same_BLC)].from()].block==m_states[t.from()].block),
             m_states[m_aut.get_transitions()[*(next_block_to_constellation->start_same_BLC)].to()].block!=index_block_B) ||
            m_aut.apply_hidden_label_map(m_aut.get_transitions()[*(next_block_to_constellation->start_same_BLC)].label())!=m_aut.apply_hidden_label_map(t.label()))
        { 
          // Make a new entry in the list next_block_to_constellation, after the current list element.
          next_block_to_constellation=
                  m_blocks[m_states[t.from()].block].block_to_constellation.
                           emplace_after(this_block_to_constellation, 
                                         this_block_to_constellation->start_same_BLC,
                                         this_block_to_constellation->start_same_BLC);
          #ifdef CHECK_COMPLEXITY_GJ
            next_block_to_constellation->work_counter = this_block_to_constellation->work_counter;
          #endif
        }
        last_element_removed=swap_in_the_doubly_linked_list_LBC_in_blocks(ti, next_block_to_constellation, this_block_to_constellation);
      }
      
      if (last_element_removed)
      {
        m_blocks[m_states[t.from()].block].block_to_constellation.erase(this_block_to_constellation);
      }
    }

    // Update the LBC list of a transition, when the from state of the transition moves
    // from block old_bi to new_bi. 
    transition_index update_the_doubly_linked_list_LBC_new_block(
               const block_index old_bi,
               const block_index new_bi,
               const transition_index ti)
    {
      const transition& t=m_aut.get_transitions()[ti];
      transition_index remaining_transition=null_transition;

      assert(m_states[t.from()].block==new_bi);
      
      linked_list<BLC_indicators>::iterator this_block_to_constellation=
                           m_transitions[ti].transitions_per_block_to_constellation;
      transition_index co_transition=null_transition;
      bool co_block_found=false;
      if (this_block_to_constellation->start_same_BLC!=m_BLC_transitions.begin())
      {
        co_transition=*(this_block_to_constellation->start_same_BLC-1);
        const transition& co_t=m_aut.get_transitions()[co_transition];
        co_block_found=m_states[co_t.from()].block==new_bi &&
                       co_t.label()==t.label() &&
                       m_blocks[m_states[co_t.to()].block].constellation==m_blocks[m_states[t.to()].block].constellation;
      }

      bool last_element_removed;

      if (!co_block_found)
      { 
        // Make a new entry in the list next_block_to_constellation;
        
        // Put inert tau's to the front. Otherwise, the new block is put after the current block. 
        linked_list<BLC_indicators>::iterator new_position;
        std::vector<transition_index>::iterator old_BLC_start=this_block_to_constellation->start_same_BLC;
        if (m_blocks[new_bi].block_to_constellation.empty() || 
            (is_inert_during_init(t) &&
             m_blocks[new_bi].constellation==m_blocks[m_states[t.to()].block].constellation))
        {
          m_blocks[new_bi].block_to_constellation.emplace_front(old_BLC_start, old_BLC_start); 
          new_position=m_blocks[new_bi].block_to_constellation.begin();
        }
        else 
        {
          new_position=m_blocks[new_bi].block_to_constellation.begin();
          new_position= m_blocks[new_bi].block_to_constellation.emplace_after(new_position,old_BLC_start, old_BLC_start);
        }
        #ifdef CHECK_COMPLEXITY_GJ
          new_position->work_counter = this_block_to_constellation->work_counter;
        #endif
        last_element_removed=swap_in_the_doubly_linked_list_LBC_in_blocks(ti, new_position, this_block_to_constellation);
      }
      else
      {
        // Move the current transition to the next list indicated by the iterator it.
        linked_list<BLC_indicators>::iterator new_BLC_block= m_transitions[co_transition].transitions_per_block_to_constellation;
        last_element_removed=swap_in_the_doubly_linked_list_LBC_in_blocks(ti, new_BLC_block, this_block_to_constellation);
      }
      
      if (last_element_removed)
      {
        // Remove this element. 
        m_blocks[old_bi].block_to_constellation.erase(this_block_to_constellation);
      }
      else
      {
        remaining_transition= *(this_block_to_constellation->start_same_BLC);
      }

      return remaining_transition;
    }

    // Set m_states[s].counter:=undefined for all s in m_R and m_U.
    void clear_state_counters(bool restrict_to_R=false)
    {
      for(const state_index si: m_R)
      {
        assert(Rmarked == m_states[si].counter); // this allows us to charge the work in this loop to setting the counter to Rmarked
        m_states[si].counter=undefined;
      }
      if (restrict_to_R)
      {
        return;
      }
      for(const state_index si: m_U_counter_reset_vector)
      {
        // this work is charged to adding a value to m_U_counter_reset_vector
        m_states[si].counter=undefined;
      }
      clear(m_U_counter_reset_vector);
    }

    // Calculate the states R in block B that can inertly reach M and split
    // B in R and B\R. The complexity is conform the smallest block of R and B\R.
    // The LBC_list and bottom states are not updated. 
    // Provide the index of the newly created block as a result. This block is the smallest of R and B\R.
    // Return in M_in_bi whether the new block bi is the one containing M.
    // This algorithm can be executed in 2 variants, numbered 1 and 2.
    // In variant 1 the marked states are given by the R_todo_vector. M_begin and M_end are 
    // irrelevant. For the unmarked states it is checked whether they do not occur in R_todo_vector before
    // adding them to U_todo_vector.
    // In variant 2 the marked states are given by M_begin to M_end and must be added one by one
    // in the R_todo_vector. For the unmarked states it is checked whether they do not occur in R_todo_vector
    // and whether they do not have an outgoing transition with action label a and constellation C. 
    template <std::size_t VARIANT,
              class MARKED_STATE_TRANSITION_ITERATOR, 
              class UNMARKED_STATE_ITERATOR>
    block_index simple_splitB(const block_index B, 
                              const MARKED_STATE_TRANSITION_ITERATOR M_begin, 
                              const MARKED_STATE_TRANSITION_ITERATOR M_end, 
                              const UNMARKED_STATE_ITERATOR M_co_begin,
                              const UNMARKED_STATE_ITERATOR M_co_end,
                              const bool initialisation,
                              const label_index a,
                              const constellation_index C,
                              bool& M_in_bi,
                              std::function<void(const state_index)> update_Ptilde)
    {
      const std::size_t B_size=number_of_states_in_block(B);
      assert(1 < B_size);
      assert(m_aut.apply_hidden_label_map(a) == a);
      // assert(VARIANT!=2 || m_R.empty());
      assert(VARIANT!=1 || m_U.empty());
      assert(VARIANT!=1 || m_U_counter_reset_vector.empty());
      typedef enum { initializing, state_checking, aborted, aborted_after_initialisation, incoming_inert_transition_checking, outgoing_action_constellation_check /*,
                     outgoing_action_constellation_check_during_initialisation*/ } status_type;
      status_type U_status=(VARIANT==1)?initializing:state_checking;
      status_type R_status=(VARIANT==1)?state_checking:initializing;
      MARKED_STATE_TRANSITION_ITERATOR M_it=M_begin; 
      UNMARKED_STATE_ITERATOR M_co_it=M_co_begin; 
      std::vector<transition>::iterator current_U_incoming_transition_iterator;
      std::vector<transition>::iterator current_U_incoming_transition_iterator_end;
      state_index current_U_outgoing_state=-1;
      outgoing_transitions_it current_U_outgoing_transition_iterator;
      outgoing_transitions_it current_U_outgoing_transition_iterator_end;
      std::vector<transition>::iterator current_R_incoming_transition_iterator;
      std::vector<transition>::iterator current_R_incoming_transition_iterator_end;

      if (VARIANT==1)  // In variant 1 the states have been added to R_todo when checking whether a split is possible. 
                       // In this case it is only necessary whether the state is Rmarked to see whether it has an outgoing
                       // marking transition. 
      {
        /* for(; M_it!=M_end; ++M_it)
        { 
          const state_index si=m_aut.get_transitions()[*M_it].from();
          if (m_states[si].counter==undefined)
          { 
            m_R.add_todo(si);
            m_states[si].counter=Rmarked;
            m_counter_reset_vector.push_back(si);
          }
        } */
        if (m_blocks[B].start_non_bottom_states == m_blocks[B].end_states)
        {
          m_R.clear_todo();
          m_U.clear_todo(); // actually it's only necessary if M_co_it==M_co_end but it doesn't hurt either.
        }
        if (M_co_it==M_co_end)
        {
          U_status=state_checking;
        }
      }
      else
      {
        if (m_blocks[B].start_non_bottom_states == m_blocks[B].end_states)
        {
          m_U.clear_todo();
        }
      }
      if (2*m_R.size()>B_size)
      {
        R_status = (VARIANT==1) ? aborted_after_initialisation : aborted;
      }
      if (2*m_U.size()>B_size)
      {
        U_status=aborted;
      }

      // Algorithm 3, line 3.2 left.

      // start coroutines. Each co-routine handles one state, and then gives control
      // to the other co-routine. The coroutines can be found sequentially below surrounded
      // by a while loop.

      while (true)
      {
        assert(U_status!=aborted || (R_status!=aborted && R_status!=aborted_after_initialisation));
#ifndef NDEBUG
        for(state_index si=0; si<m_states.size(); ++si)
        {
          if (m_states[si].block != B)
          {
            assert(!m_R.find(si));
            assert(!m_U.find(si));
          }
          else
          {
            switch(m_states[si].counter)
            {
            case undefined:  if (0 != m_states[si].no_of_outgoing_inert_transitions)  assert(!m_U.find(si));
                             assert(!m_R.find(si)); break;
            case Rmarked:    assert( m_R.find(si)); assert(!m_U.find(si)); break;
            case 0:          assert(!m_R.find(si)); break; // It can happen that the state is in U or is not in U
            default:         assert(!m_R.find(si)); assert(!m_U.find(si)); break;
            }
          }
        }
#endif
        // The code for the right co-routine. 
        switch (R_status)
        {
          case initializing:
          {
            assert(2 == VARIANT);
            // Algorithm 3, line 3.3, right.
              const state_index si= m_aut.get_transitions()[*M_it].from();
              mCRL2complexity_gj(&m_transitions[*M_it], add_work(check_complexity::simple_splitB_R__handle_transition_from_R_state, 1), *this);
              assert(m_aut.apply_hidden_label_map(m_aut.get_transitions()[*M_it].label()) == a);
              assert(!m_branching || !m_aut.is_tau(a) || m_states[m_aut.get_transitions()[*M_it].from()].block != m_states[m_aut.get_transitions()[*M_it].to()].block || (m_preserve_divergence && m_aut.get_transitions()[*M_it].from() == m_aut.get_transitions()[*M_it].to()));
              assert(m_blocks[m_states[m_aut.get_transitions()[*M_it].to()].block].constellation == C);
              ++M_it;
              // R_todo.insert(si);
              if (m_states[si].counter!=Rmarked)
              {
                // assert(undefined == m_states[si].counter); -- does not always hold, because it may be a nonbottom state with a tau-transition to a U-state, or even with all tau-transitions to U-states
                // assert(0 < m_states[si].no_of_outgoing_inert_transitions); -- David thinks that all bottom states should already be marked before the coroutines start; however, this does not hold during bottom state splits. David is not sure whether this is correct. What happens if a new bottom state has a transition in the splitter?
                assert(!m_R.find(si));
                m_R.add_todo(si);
                /* if (m_states[si].counter==undefined)
                {
                  m_counter_reset_vector.push_back(si);
                } */
                m_states[si].counter=Rmarked;
//std::cerr << "R_todo1 insert: " << si << "\n";
                if (2*m_R.size()>B_size) 
                {
                  R_status=aborted;
                  break;
                }
              }
              else assert(m_R.find(si));
              assert(!m_U.find(si));
              if (M_it==M_end)
              {
                if (m_blocks[B].start_non_bottom_states == m_blocks[B].end_states)
                {
                  m_R.clear_todo();
                }
                R_status=state_checking;
              }
            break;
          }
          case state_checking: 
          {
            if (m_R.todo_is_empty())
            {
//std::cerr << "R empty: " << "\n";
              // split_block B into R and B\R.
              assert(m_R.size()>0);
              clear_state_counters();
              /* for(const state_index si: m_counter_reset_vector)
              {
                m_states[si].counter=undefined;
              }
              clear(m_counter_reset_vector); */
              m_U.clear();
              block_index block_index_of_R=split_block_B_into_R_and_BminR(B, m_R, update_Ptilde);
              m_R.clear();
              M_in_bi=true;
              return block_index_of_R;
            }
            else
            {
              assert(m_blocks[B].start_non_bottom_states < m_blocks[B].end_states);
              const state_index s=m_R.move_from_todo();
              mCRL2complexity_gj(&m_states[s], add_work(check_complexity::simple_splitB_R__find_predecessors, 1), *this);
//std::cerr << "R insert: " << s << "\n";
              assert(m_states[s].block == B);
              // current_R_incoming_transition_iterator_end=m_states[s].start_incoming_non_inert_transitions;
              if (s+1==m_states.size())
              {
                current_R_incoming_transition_iterator_end=m_aut.get_transitions().end();
              }
              else
              {
                current_R_incoming_transition_iterator_end=m_states[s+1].start_incoming_transitions;
              }
              current_R_incoming_transition_iterator=m_states[s].start_incoming_transitions;
              if (current_R_incoming_transition_iterator!=current_R_incoming_transition_iterator_end &&
                  m_aut.is_tau(m_aut.apply_hidden_label_map(current_R_incoming_transition_iterator->label())))
              {
                R_status=incoming_inert_transition_checking;
              }
            }
            break;
          }
          case incoming_inert_transition_checking:
          {
              assert(current_R_incoming_transition_iterator!=current_R_incoming_transition_iterator_end);
              const transition& tr= *current_R_incoming_transition_iterator;
              assert(m_aut.is_tau(m_aut.apply_hidden_label_map(tr.label())));
              mCRL2complexity_gj(&m_transitions[std::distance(m_aut.get_transitions().begin(), current_R_incoming_transition_iterator)],
                        add_work(check_complexity::simple_splitB_R__handle_transition_to_R_state, 1), *this);
              assert(m_states[tr.to()].block == B);
              if (m_states[tr.from()].block==B && !(m_preserve_divergence && tr.from() == tr.to()))
              {
                if (m_states[tr.from()].counter!=Rmarked)
                {
                  assert(!m_R.find(tr.from()));
//std::cerr << "R_todo2 insert: " << tr.from() << "\n";
                  m_R.add_todo(tr.from());
                  /* if (m_states[tr.from()].counter==undefined)
                  {
                    m_counter_reset_vector.push_back(tr.from());
                  } */
                  m_states[tr.from()].counter=Rmarked;

                  // Algorithm 3, line 3.10 and line 3.11, right.
                  if (2*m_R.size()>B_size)
                  {
                    R_status=aborted_after_initialisation;
                    break;
                  }
                }
                else assert(m_R.find(tr.from()));
                assert(!m_U.find(tr.from()));
              }
              ++current_R_incoming_transition_iterator;
              if (current_R_incoming_transition_iterator==current_R_incoming_transition_iterator_end ||
                  !m_aut.is_tau(m_aut.apply_hidden_label_map(current_R_incoming_transition_iterator->label())))
              {
                R_status=state_checking;
              }
              break;
          }
          default:
          {
            assert(aborted == R_status || aborted_after_initialisation == R_status);
            break;
          }
        }

#ifndef NDEBUG
        for(state_index si=0; si<m_states.size(); ++si)
        {
          if (m_states[si].block != B)
          {
            assert(!m_R.find(si));
            assert(!m_U.find(si));
          }
          else
          {
            switch(m_states[si].counter)
            {
            case undefined:  if (0 != m_states[si].no_of_outgoing_inert_transitions)  assert(!m_U.find(si));
                             assert(!m_R.find(si)); break;
            case Rmarked:    assert( m_R.find(si)); assert(!m_U.find(si)); break;
            case 0:          assert(!m_R.find(si)); break; // It can happen that the state is in U or is not in U
            default:         assert(!m_R.find(si)); assert(!m_U.find(si)); break;
            }
          }
        }
#endif 
        // The code for the left co-routine. 
        switch (U_status) 
        {
          case initializing: // Only executed in VARIANT 1.
          {
//std::cerr << "U_init\n";
            assert(1 == VARIANT);
            // Algorithm 3, line 3.3 left.
            assert(M_co_it != M_co_end);
            for (;;)
            {
              const state_index si=*M_co_it;
              M_co_it++;
              assert(0 == m_states[si].no_of_outgoing_inert_transitions);
              assert(!m_U.find(si));
              
              // if (m_states[si].counter!=Rmarked)
              if (m_states[si].counter==undefined)
              {
                mCRL2complexity_gj(&m_states[si], add_work(check_complexity::simple_splitB_U__find_bottom_state, 1), *this);
                /* if (VARIANT==2 && !(R_status==state_checking || R_status==incoming_inert_transition_checking || R_status==aborted_after_initialisation))
                {
                  current_U_outgoing_state=si;
                  current_U_outgoing_transition_iterator=m_states[si].start_outgoing_transitions;
                  U_status=outgoing_action_constellation_check_during_initialisation;
                  break;
                } */

                assert(!m_R.find(si));
                // This is for VARIANT 1.
                // if (m_states[si].counter==undefined)
                {
                  m_U.add_todo(si);
//std::cerr << "U_todo1 insert: " << si << "   " << m_U.size() << "    " << B_size << "\n";
                  // Algorithm 3, line 3.10 and line 3.11 left.
                  if (2*m_U.size()>B_size)
                  {
                    U_status=aborted;
                    break;
                  }
                }
                if (M_co_it == M_co_end)
                {
                  if (m_blocks[B].start_non_bottom_states == m_blocks[B].end_states)
                  {
                    m_U.clear_todo();
                  }
                  U_status = state_checking;
                }
                // We have executed one step that is actually assigned to a U-state,
                // so we should leave the for(;;) loop and yield to the other coroutine.
                break;
              }
              else
              {
                assert(Rmarked == m_states[si].counter);
                assert(m_R.find(si));
                // We cannot assign the work to a U-state, but as si must have
                // a transition in the splitter, we assign it to that transition.
                add_work_to_same_saC(initialisation, si, a, C,
                        check_complexity::simple_splitB__do_not_add_state_with_transition_in_splitter_to_U,
                        check_complexity::log_n - check_complexity::ilog2(number_of_states_in_constellation(C)));
                // To keep the balance between U and R, we have to continue
                // working on U until we find a unit of work that can be assigned to U.
                if (M_co_it == M_co_end)
                {
                  // We have reached the end of the bottom states without finding an (additional) U-state.
                  // To keep the balance, we have to execute one action from the next case (i.e. case state_checking).
                  if (m_blocks[B].start_non_bottom_states == m_blocks[B].end_states)
                  {
                    m_U.clear_todo();
                  }
                  U_status = state_checking;
                  goto do_state_checking;
                }
              }
            }
            break;
          }
/*          case outgoing_action_constellation_check_during_initialisation:
// David suggests: this should not be necessary, because the bottom states can always be split
// before the two coroutines start.
// In fact, if current_U_outgoing_state is already a bottom state
// and does not have a transition to the splitter,
// this check takes too long.
// The outgoing-action-constellation-check is only allowed for states
// that can become NEW bottom states.
          {
//std::cerr << "U_outg_actconstcheckduringit\n";
            if (current_U_outgoing_transition_iterator==m_outgoing_transitions.end() ||
                (current_U_outgoing_state+1<m_states.size() &&
                    current_U_outgoing_transition_iterator==m_states[current_U_outgoing_state+1].start_outgoing_transitions))
            {
              assert(!m_U.find(current_U_outgoing_state));
//std::cerr << "U_todo2 insert: " << current_U_outgoing_state << "\n";
              m_U.add_todo(current_U_outgoing_state);
              m_states[current_U_outgoing_state].counter=0;
              m_U_counter_reset_vector.push_back(current_U_outgoing_state);
              // Algorithm 3, 
              if (2*m_U.size()>B_size)
              {
                U_status=aborted;
                break;
              }
              else
              {
                U_status=initializing;
              }
              break;
            }
            else
            {
              const transition& t_local=m_aut.get_transitions()[current_U_outgoing_transition_iterator->transition];
              if (m_aut.apply_hidden_label_map(t_local.label())==a && m_blocks[m_states[t_local.to()].block].constellation==C)
              {
                // This state must be blocked.
                U_status=initializing;
                // m_states[current_U_outgoing_state].counter=undefined;
                break;
              }
              current_U_outgoing_transition_iterator=current_U_outgoing_transition_iterator->start_same_saC; // This is an optimisation.
              current_U_outgoing_transition_iterator++;
            }
            break;
          } */
          do_state_checking:
          case state_checking:
          {
//std::cerr << "U_state_checking\n";
            
            // Algorithm 3, line 3.23 and line 3.24, left. 
            if (m_U.todo_is_empty())
            {
//std::cerr << "U_todo empty: " << "\n";
              assert(!m_U.empty());
              // split_block B into U and B\U.
              assert(m_U.size()>0);
              clear_state_counters();
              /* for(const state_index si: m_counter_reset_vector)
              {
                m_states[si].counter=undefined;
              }
              clear(m_counter_reset_vector); */
              m_R.clear();
              block_index block_index_of_U=split_block_B_into_R_and_BminR(B, m_U, update_Ptilde);
              m_U.clear();
              M_in_bi = false;
              return block_index_of_U;
            }
            else
            {
              assert(m_blocks[B].start_non_bottom_states < m_blocks[B].end_states);
              // const state_index s=U_todo.extract(U_todo.begin()).value();
              const state_index s=m_U.move_from_todo();
              assert(!m_R.find(s));
              mCRL2complexity_gj(&m_states[s], add_work(check_complexity::simple_splitB_U__find_predecessors, 1), *this);
//std::cerr << "U insert/ U_todo_remove: " << s << "\n";
              current_U_incoming_transition_iterator=m_states[s].start_incoming_transitions;
              if (s+1==m_states.size())
              {
                current_U_incoming_transition_iterator_end=m_aut.get_transitions().end(); 
              }
              else
              {
                current_U_incoming_transition_iterator_end=m_states[s+1].start_incoming_transitions;
              }
              if (current_U_incoming_transition_iterator != current_U_incoming_transition_iterator_end &&
                  m_aut.is_tau(m_aut.apply_hidden_label_map(current_U_incoming_transition_iterator->label())))
              {
                U_status=incoming_inert_transition_checking;
              }
              break;
            }
          }
          case incoming_inert_transition_checking:
          {
//std::cerr << "U_incoming_inert_transition_checking\n";
            // Algorithm 3, line 3.8, left.
            assert(current_U_incoming_transition_iterator != current_U_incoming_transition_iterator_end);
            assert(m_aut.is_tau(m_aut.apply_hidden_label_map(current_U_incoming_transition_iterator->label())));
            // Check one incoming transition.
            // Algorithm 3, line 3.12, left.
            mCRL2complexity_gj(&m_transitions[std::distance(m_aut.get_transitions().begin(), current_U_incoming_transition_iterator)], add_work(check_complexity::simple_splitB_U__handle_transition_to_U_state, 1), *this);
            state_index from=current_U_incoming_transition_iterator->from();
            const state_index to_state=current_U_incoming_transition_iterator->to();
            assert(m_states[to_state].block == B);
            current_U_incoming_transition_iterator++;
//std::cerr << "FROM " << from << "\n";
            if (m_states[from].block==B && !(m_preserve_divergence && from == to_state))
            {
              assert(!m_U.find(from));
              if (m_states[from].counter != Rmarked)
              {
                if (m_states[from].counter==undefined) // count(from) is undefined;
                {
                  // Algorithm 3, line 3.13, left.
                  // Algorithm 3, line 3.15 and 3.18, left.
                  m_states[from].counter=m_states[from].no_of_outgoing_inert_transitions-1;
//std::cerr << "COUNTER " << m_states[from].counter << "\n";
                  m_U_counter_reset_vector.push_back(from);
                }
                else
                {
                  // Algorithm 3, line 3.18, left.
                  assert(std::find(m_U_counter_reset_vector.begin(), m_U_counter_reset_vector.end(), from) != m_U_counter_reset_vector.end());
                  assert(m_states[from].counter>0);
                  m_states[from].counter--;
                }
                // Algorithm 3, line 3.19, left.
                if (m_states[from].counter==0)
                {
                  if (VARIANT==2 && !(R_status==state_checking || R_status==incoming_inert_transition_checking || R_status==aborted_after_initialisation))
                  {
                    // Start searching for an outgoing transition with action a to constellation C. 
                    current_U_outgoing_state=from;
                    current_U_outgoing_transition_iterator = m_states[current_U_outgoing_state].start_outgoing_transitions;
                    current_U_outgoing_transition_iterator_end = (current_U_outgoing_state+1 >= m_states.size() ? m_outgoing_transitions.end() : m_states[current_U_outgoing_state+1].start_outgoing_transitions);
                    assert(current_U_outgoing_transition_iterator != current_U_outgoing_transition_iterator_end);
                    U_status=outgoing_action_constellation_check;
                    break;
                  }
                  else
                  {
                    // VARIANT=1. The state can be added to U_todo. 
//std::cerr << "U_todo3 insert: " << from << "   " << m_U.size() << "    " << B_size << "\n";
                    m_U.add_todo(from);
                    // Algorithm 3, line 3.10 and line 3.11 left. 
                    if (2*m_U.size()>B_size)  
                    {
                      U_status=aborted;
                      break;
                    }
                  }
                }
              }
              else assert(m_R.find(from));
            }
            if (current_U_incoming_transition_iterator == current_U_incoming_transition_iterator_end ||
                !m_aut.is_tau(m_aut.apply_hidden_label_map(current_U_incoming_transition_iterator->label())))
            {
              U_status = state_checking;
            }
            break;
          }
          case outgoing_action_constellation_check:
          {
//std::cerr << "U_outgoing_action_constellation_check\n";
            assert(current_U_outgoing_transition_iterator != current_U_outgoing_transition_iterator_end);
            #ifdef CHECK_COMPLEXITY_GJ
              mCRL2complexity_gj((&m_transitions[initialisation?current_U_outgoing_transition_iterator->transition:m_BLC_transitions[current_U_outgoing_transition_iterator->transition]]), add_work(check_complexity::simple_splitB_U__handle_transition_from_potential_U_state, 1), *this);
              // This is one step in the coroutine, so we should assign the work to exactly one transition.
              // But to make sure, we also mark the other transitions that we skipped in the optimisation.
              for (outgoing_transitions_it out_it = current_U_outgoing_transition_iterator; out_it != current_U_outgoing_transition_iterator->start_same_saC; )
              {
                ++out_it;
                mCRL2complexity_gj(&m_transitions[initialisation?out_it->transition:m_BLC_transitions[out_it->transition]], add_work_notemporary(check_complexity::simple_splitB_U__handle_transition_from_potential_U_state, 1), *this);
              }
            #endif
            const transition& t_local=m_aut.get_transitions()
                          [initialisation
                               ?current_U_outgoing_transition_iterator->transition
                               :m_BLC_transitions[current_U_outgoing_transition_iterator->transition]];
            current_U_outgoing_transition_iterator=current_U_outgoing_transition_iterator->start_same_saC; // This is an optimisation.
            ++current_U_outgoing_transition_iterator;

            assert(t_local.from() == current_U_outgoing_state);
            assert(m_branching);
            if (m_blocks[m_states[t_local.to()].block].constellation==C &&
                (m_aut.is_tau(a) ? m_aut.is_tau(m_aut.apply_hidden_label_map(t_local.label())) &&
                                   (m_states[t_local.to()].block != B || (m_preserve_divergence && t_local.from() == t_local.to()))
                                 : t_local.label() == a))
            {
                // This state must be blocked.
            }
            else if (current_U_outgoing_transition_iterator == current_U_outgoing_transition_iterator_end)
            {
              // assert(U.find(current_U_outgoing_state)==U.end());
              assert(!m_U.find(current_U_outgoing_state));
//std::cerr << "U_todo4 insert: " << current_U_outgoing_state << "   " << m_U.size() << "    " << B_size << "\n";
              m_U.add_todo(current_U_outgoing_state);
              // Algorithm 3, line 3.10 and line 3.11 left. 
              if (2*m_U.size()>B_size)
              {
                U_status=aborted;
                break;
              }
            }
            else  break;

            U_status = incoming_inert_transition_checking;
            if (current_U_incoming_transition_iterator == current_U_incoming_transition_iterator_end ||
                !m_aut.is_tau(m_aut.apply_hidden_label_map(current_U_incoming_transition_iterator->label())))
            {
              U_status = state_checking;
            }
            break;
          }
          default:
          {
            assert(U_status == aborted);
            break;
          }
        }
      }
      assert(0);
    }

    void make_transition_non_inert(const transition& t)
    {
      //const transition& t=m_aut.get_transitions()[ti];
      assert(is_inert_during_init(t));
      assert(m_states[t.to()].block!=m_states[t.from()].block);
      // Move the transition indicated by tti to the non inert transitions in m_incoming_transitions.
      /* state_type_gj& to=m_states[m_aut.get_transitions()[ti].to()];
      to.start_incoming_non_inert_transitions--;
      std::vector<transition_index>::iterator move_to1=to.start_incoming_non_inert_transitions;
      std::vector<transition_index>::iterator current_position1=m_transitions[ti].ref_incoming_transitions;
      if (move_to1!=current_position1)
      {
        std::swap(*move_to1,*current_position1);
        m_transitions[*move_to1].ref_incoming_transitions=move_to1;
        m_transitions[*current_position1].ref_incoming_transitions=current_position1;
      } */

      m_states[t.from()].no_of_outgoing_inert_transitions--;
    }

    // Split block B in R, being the inert-tau transitive closure of M contains 
    // states that must be in block, and M\R. M_nonmarked, minus those in unmarked_blocker, are those in the other block. 
    // The splitting is done in time O(min(|R|,|B\R|). Return the block index of the smallest
    // block, which is newly created. Indicate in M_in_new_block whether this new block contains M.
    template <std::size_t VARIANT,
              class MARKED_STATE_TRANSITION_ITERATOR,
              class UNMARKED_STATE_ITERATOR>
    block_index splitB(const block_index B, 
                       const MARKED_STATE_TRANSITION_ITERATOR M_begin, 
                       const MARKED_STATE_TRANSITION_ITERATOR M_end, 
                       const UNMARKED_STATE_ITERATOR M_co_begin,
                       const UNMARKED_STATE_ITERATOR M_co_end,
                       const label_index a,
                       const constellation_index C,
                       // const std::function<bool(state_index)>& unmarked_blocker,
                       bool& M_in_new_block,
                       std::function<void(const block_index, const block_index, const transition_index, const transition_index)> 
                                                 update_block_label_to_cotransition,
                       // bool update_LBC_list=true,
                       const bool initialisation=false,
                       std::function<void(const transition_index, const transition_index, const block_index)> process_transition=
                                                        [](const transition_index, const transition_index, const block_index){},
                       std::function<void(const state_index)> update_Ptilde=
                                                        [](const state_index){})
    {
// mCRL2log(log::debug) << "splitB<" << VARIANT << ">(" << m_blocks[B].debug_id(*this) << ",{ ";
// for(MARKED_STATE_TRANSITION_ITERATOR s=M_begin; s!=M_end; ++s){ mCRL2log(log::debug) << m_aut.get_transitions()[*s].from() << ' '; }
// mCRL2log(log::debug) << "},{ ";
// for(UNMARKED_STATE_ITERATOR s=M_co_begin; s!=M_co_end; ++s){ mCRL2log(log::debug) << *s << ' '; }
// if (m_aut.num_action_labels() == a) { mCRL2log(log::debug) << "},tau-self-loops,"; }
// else { mCRL2log(log::debug) << "}," << pp(m_aut.action_label(a)) << ','; }
// mCRL2log(log::debug) << m_constellations[C].debug_id(*this) << ",...)\n";
      if (1 >= number_of_states_in_block(B))
      {
        M_in_new_block = false;
        return -1;
      }
      assert(M_begin!=M_end && M_co_begin!=M_co_end);
      block_index bi=simple_splitB<VARIANT, MARKED_STATE_TRANSITION_ITERATOR, UNMARKED_STATE_ITERATOR>
                 (B, M_begin, M_end, M_co_begin, M_co_end, initialisation, a, C, M_in_new_block,update_Ptilde);

//std::cerr << "Split block of size " << number_of_states_in_block(B) + number_of_states_in_block(bi) << " taking away " << number_of_states_in_block(bi) << " states\n";
      assert(number_of_states_in_block(B)+1 >= number_of_states_in_block(bi));
// David asks: why the ``+1''? Is that really necessary?

      // Because we visit all states of block bi and almost all their incoming and outgoing transitions,
      // we subsume all this bookkeeping in a single block counter:
      mCRL2complexity_gj(&m_blocks[bi], add_work(check_complexity::splitB__update_BLC_of_smaller_subblock, check_complexity::log_n - check_complexity::ilog2(number_of_states_in_block(bi))), *this);
      // Update the LBC_list, and bottom states, and invariant on inert transitions.
      // Recall new LBC positions.
      // std::unordered_map< std::pair <label_index, constellation_index>, 
      //                      linked_list<BLC_indicators>::iterator> new_LBC_list_entries;
      for(typename std::vector<state_index>::iterator ssi=m_blocks[bi].start_bottom_states;
                                                      ssi!=m_blocks[bi].end_states;
                                                      ++ssi)
      {
        const state_index si=*ssi;
        state_type_gj& s= m_states[si];
        // mCRL2complexity_gj(s, add_work(..., max_bi_counter), *this);
            // is subsumed in the above call
        s.block=bi;

        // Situation below is only relevant if M_in_new_block;
        if (M_in_new_block && ssi >= m_blocks[bi].start_non_bottom_states)
        {
          // si is a non_bottom_state in the smallest block containing M..
          bool non_bottom_state_becomes_bottom_state = true;
        
          const outgoing_transitions_it end_it=((*ssi)+1>=m_states.size())?m_outgoing_transitions.end():m_states[(*ssi)+1].start_outgoing_transitions;
          for(outgoing_transitions_it ti=s.start_outgoing_transitions; ti!=end_it; ti++)
          {       
            // mCRL2complexity_gj(&m_transitions[ti->transition], add_work(..., max_bi_counter), *this);
                // is subsumed in the above call
            const transition& t=m_aut.get_transitions()[initialisation?ti->transition:m_BLC_transitions[ti->transition]];
            assert(t.from() == *ssi);
            assert(m_branching);
            if (m_aut.is_tau(m_aut.apply_hidden_label_map(t.label())) && !(m_preserve_divergence && t.from() == t.to()))
            { 
              if  (m_states[t.to()].block==B)
              {
                // This is a transition that has become non-inert.
                make_transition_non_inert(t);
                // The LBC-list of this transition will be updated below, as it is now non-inert. 
              }
              else if (m_states[t.to()].block==bi)
              {
                non_bottom_state_becomes_bottom_state=false; // There is an outgoing inert tau. State remains non-bottom.
              }
            }
          }
          if (non_bottom_state_becomes_bottom_state)
          {
            // The state at si has become a bottom_state.
            assert(find(m_P.begin(), m_P.end(), si)==m_P.end());
            m_P.push_back(si);
            // Move this former non bottom state to the bottom states.
            if (ssi!=m_blocks[bi].start_non_bottom_states)
            {
              // Note that the call below damages the value of *ssi. Here it is not anymore equal to si.
              swap_states_in_states_in_block(ssi, m_blocks[bi].start_non_bottom_states);
            }
            m_blocks[bi].start_non_bottom_states++;
          }
        }

        if (!initialisation)  // update the BLC_lists.
        {
          const outgoing_transitions_it end_it=((si+1)==m_states.size())?m_outgoing_transitions.end():m_states[si+1].start_outgoing_transitions;
          for(outgoing_transitions_it ti=s.start_outgoing_transitions; ti!=end_it; ti++)
          {       
            // mCRL2complexity_gj(&m_transitions[ti->transition], add_work(..., max_bi_counter), *this);
                // is subsumed in the above call
            // transition_index old_remaining_transition=update_the_doubly_linked_list_LBC_new_block(B, bi, ti->transition, new_LBC_list_entries);
            transition_index old_remaining_transition=update_the_doubly_linked_list_LBC_new_block(B, bi, m_BLC_transitions[ti->transition]);
            process_transition(m_BLC_transitions[ti->transition], old_remaining_transition, B);
            update_block_label_to_cotransition(B, bi, m_BLC_transitions[ti->transition], old_remaining_transition);
          }
        }
        
        /*
        // Investigate the incoming tau transitions. 
        if (!M_in_new_block)
        {
          for(std::vector<transition_index>::iterator ti=s.start_incoming_inert_transitions; 
                      ti!=s.start_incoming_non_inert_transitions; )
          {       
            const transition& t=m_aut.get_transitions()[*ti];
            // mCRL2complexity_gj(&m_transitions[*ti], add_work(..., max_bi_counter), *this);
                // is subsumed in the above call
            assert(is_inert_during_init(t));
            assert(m_states[t.to()].block==bi);
            if (m_states[t.from()].block==B)
            { 
              // This transition did become non-inert.
              make_transition_non_inert(*ti);
              
              // Check whether from is a new bottom state.
              const state_index from=t.from();
              if (m_states[from].no_of_outgoing_inert_transitions==0)
              {
                // This state has not outgoing inert transitions. It becomes a bottom state. 
                assert(find(m_P.begin(), m_P.end(), from)==m_P.end());
                m_P.push_back(from);
                // Move this former non bottom state to the bottom states.
                block_index temp_bi=m_states[from].block;
                if (m_states[from].ref_states_in_blocks!=m_blocks[temp_bi].start_non_bottom_states)
                {
                  swap_states_in_states_in_block(m_states[from].ref_states_in_blocks, m_blocks[temp_bi].start_non_bottom_states);
                }
                m_blocks[temp_bi].start_non_bottom_states++;
              }
            }
            else
            {
              ti++;
            }
          }
        } */
      
        // Investigate the incoming formerly inert tau transitions.
        if (!M_in_new_block && m_blocks[B].start_non_bottom_states < m_blocks[B].end_states)
        {
          // for(std::vector<transition_index>::iterator ti=s.start_incoming_inert_transitions; 
          //            ti!=s.start_incoming_non_inert_transitions; )
          const std::vector<transition>::iterator it_end = si+1>=m_states.size() ? m_aut.get_transitions().end() : m_states[si+1].start_incoming_transitions;
          for(std::vector<transition>::iterator it=s.start_incoming_transitions; 
                        it!=it_end; it++)
          {       
            // const transition& t=m_aut.get_transitions()[*ti];
            const transition& t=*it;
            // mCRL2complexity_gj(&m_transitions[std::distance(m_aut.get_transitions().begin(), it)], add_work(..., max_bi_counter), *this);
                // is subsumed in the above call
//std::cerr << "TRANSITION XXX " << ptr(t) << "    " << m_states[t.to()].block << "   " << bi << "\n";
            assert(t.to() == si);
            if (!m_aut.is_tau(m_aut.apply_hidden_label_map(t.label())))
            {
              break; // All tau transitions have been investigated. 
            }

            if (m_states[t.from()].block==B && !(m_preserve_divergence && t.from() == si))
            { 
              // This transition did become non-inert.
              make_transition_non_inert(*it);
              
              // Check whether from is a new bottom state.
              const state_index from=t.from();
              if (m_states[from].no_of_outgoing_inert_transitions==0)
              {
                // This state has no more outgoing inert transitions. It becomes a bottom state.
                assert(find(m_P.begin(), m_P.end(), from)==m_P.end());
                m_P.push_back(from);
                // Move this former non bottom state to the bottom states.
                if (m_states[from].ref_states_in_blocks!=m_blocks[B].start_non_bottom_states)
                {
                  swap_states_in_states_in_block(m_states[from].ref_states_in_blocks, m_blocks[B].start_non_bottom_states);
                }
                m_blocks[B].start_non_bottom_states++;
              }
            }
            /* else
            {
              ti++;
            } */
          }
        }
      }

      #ifdef CHECK_COMPLEXITY_GJ
        unsigned const max_block(check_complexity::log_n - check_complexity::ilog2(number_of_states_in_block(bi)));
        if (M_in_new_block)
        {
          // account for the work in R
          for (typename std::vector<state_index>::iterator s = m_blocks[bi].start_bottom_states ;
                              s != m_blocks[bi].end_states ; ++s)
          {
            mCRL2complexity_gj(&m_states[*s], finalise_work(check_complexity::simple_splitB_R__find_predecessors, check_complexity::simple_splitB__find_predecessors_of_R_or_U_state, max_block), *this);
            // incoming tau-transitions of s
            std::vector<transition>::iterator ti_end = *s + 1 >= m_states.size() ? m_aut.get_transitions().end() : m_states[*s+1].start_incoming_transitions;
            for (std::vector<transition>::iterator ti = m_states[*s].start_incoming_transitions; ti != ti_end; ++ti)
            {
              if (!m_aut.is_tau(m_aut.apply_hidden_label_map(ti->label())))  break;
              mCRL2complexity_gj(&m_transitions[std::distance(m_aut.get_transitions().begin(), ti)], finalise_work(check_complexity::simple_splitB_R__handle_transition_to_R_state, check_complexity::simple_splitB__handle_transition_to_R_or_U_state, max_block), *this);
            }
            // outgoing transitions of s
            for (outgoing_transitions_it ti = (*s+1 >= m_states.size() ? m_outgoing_transitions.end() : m_states[*s+1].start_outgoing_transitions);
                      ti-- != m_states[*s].start_outgoing_transitions; )
            {
              mCRL2complexity_gj(&m_transitions[initialisation?ti->transition:m_BLC_transitions[ti->transition]], finalise_work(check_complexity::simple_splitB_R__handle_transition_from_R_state, check_complexity::simple_splitB__handle_transition_from_R_or_U_state, max_block), *this);
              // We also need to cancel the work on outgoing transitions of U-state candidates that turned out to be new bottom states:
              mCRL2complexity_gj(&m_transitions[initialisation?ti->transition:m_BLC_transitions[ti->transition]], cancel_work(check_complexity::simple_splitB_U__handle_transition_from_potential_U_state), *this);
            }
          }
          // ensure not too much work has been done on U
          for (typename std::vector<state_index>::iterator s = m_blocks[B].start_bottom_states ;
                              s != m_blocks[B].end_states ; ++s )
          {
            mCRL2complexity_gj(&m_states[*s], cancel_work(check_complexity::simple_splitB_U__find_bottom_state), *this);
            mCRL2complexity_gj(&m_states[*s], cancel_work(check_complexity::simple_splitB_U__find_predecessors), *this);
            // incoming tau-transitions of s
            std::vector<transition>::iterator ti_end = *s + 1 >= m_states.size() ? m_aut.get_transitions().end() : m_states[*s+1].start_incoming_transitions;
            for (std::vector<transition>::iterator ti = m_states[*s].start_incoming_transitions; ti != ti_end; ++ti)
            {
              if (!m_aut.is_tau(m_aut.apply_hidden_label_map(ti->label())))  break;
              mCRL2complexity_gj(&m_transitions[std::distance(m_aut.get_transitions().begin(), ti)], cancel_work(check_complexity::simple_splitB_U__handle_transition_to_U_state), *this);
            }
            // outgoing transitions of s
            for (outgoing_transitions_it ti = (*s + 1 >= m_states.size() ? m_outgoing_transitions.end() : m_states[*s+1].start_outgoing_transitions);
                      ti-- != m_states[*s].start_outgoing_transitions; )
            {
              mCRL2complexity_gj(&m_transitions[initialisation?ti->transition:m_BLC_transitions[ti->transition]], cancel_work(check_complexity::simple_splitB_U__handle_transition_from_potential_U_state), *this);
            }
          }
        }
        else
        {
          // account for the work in U
          for (typename std::vector<state_index>::iterator s = m_blocks[bi].start_bottom_states ;
                              s != m_blocks[bi].end_states ; ++s)
          {
            mCRL2complexity_gj(&m_states[*s], finalise_work(check_complexity::simple_splitB_U__find_bottom_state, check_complexity::simple_splitB__find_bottom_state, max_block), *this);
            mCRL2complexity_gj(&m_states[*s], finalise_work(check_complexity::simple_splitB_U__find_predecessors, check_complexity::simple_splitB__find_predecessors_of_R_or_U_state, max_block), *this);
            // incoming tau-transitions of s
            std::vector<transition>::iterator ti_end = *s + 1 >= m_states.size() ? m_aut.get_transitions().end() : m_states[*s+1].start_incoming_transitions;
            for (std::vector<transition>::iterator ti = m_states[*s].start_incoming_transitions; ti != ti_end; ++ti)
            {
              if (!m_aut.is_tau(m_aut.apply_hidden_label_map(ti->label())))  break;
              mCRL2complexity_gj(&m_transitions[std::distance(m_aut.get_transitions().begin(), ti)], finalise_work(check_complexity::simple_splitB_U__handle_transition_to_U_state, check_complexity::simple_splitB__handle_transition_to_R_or_U_state, max_block), *this);
            }
            // outgoing transitions of s
            for (outgoing_transitions_it ti = *s + 1 >= m_states.size() ? m_outgoing_transitions.end() : m_states[*s+1].start_outgoing_transitions;
                      ti-- != m_states[*s].start_outgoing_transitions; )
            {
              mCRL2complexity_gj(&m_transitions[initialisation?ti->transition:m_BLC_transitions[ti->transition]], finalise_work(check_complexity::simple_splitB_U__handle_transition_from_potential_U_state, check_complexity::simple_splitB__handle_transition_from_R_or_U_state, max_block), *this);
            }
          }
          // ensure not too much work has been done on R
          for (typename std::vector<state_index>::iterator s = m_blocks[B].start_bottom_states ;
                              s != m_blocks[B].end_states ; ++s )
          {
            mCRL2complexity_gj(&m_states[*s], cancel_work(check_complexity::simple_splitB_R__find_predecessors), *this);
            // incoming tau-transitions of s
            std::vector<transition>::iterator ti_end = *s + 1 >= m_states.size() ? m_aut.get_transitions().end() : m_states[*s+1].start_incoming_transitions;
            for (std::vector<transition>::iterator ti = m_states[*s].start_incoming_transitions; ti != ti_end; ++ti)
            {
              if (!m_aut.is_tau(m_aut.apply_hidden_label_map(ti->label())))  break;
              mCRL2complexity_gj(&m_transitions[std::distance(m_aut.get_transitions().begin(), ti)], cancel_work(check_complexity::simple_splitB_R__handle_transition_to_R_state), *this);
            }
            // outgoing transitions of s
            for (outgoing_transitions_it ti = (*s + 1 >= m_states.size() ? m_outgoing_transitions.end() : m_states[*s+1].start_outgoing_transitions);
                      ti-- != m_states[*s].start_outgoing_transitions; )
            {
              mCRL2complexity_gj(&m_transitions[initialisation?ti->transition:m_BLC_transitions[ti->transition]], cancel_work(check_complexity::simple_splitB_R__handle_transition_from_R_state), *this);
              // We also need to move the work on outgoing transitions of U-state candidates that turned out to be new bottom states:
              mCRL2complexity_gj(&m_transitions[initialisation?ti->transition:m_BLC_transitions[ti->transition]], finalise_work(check_complexity::simple_splitB_U__handle_transition_from_potential_U_state, check_complexity::simple_splitB__test_outgoing_transitions_found_new_bottom_state,
                      s < m_blocks[B].start_non_bottom_states ? 1 : 0), *this);
            }
          }
        }
        check_complexity::check_temporary_work();
      #endif // ifdef CHECK_COMPLEXITY_GJ

      return bi;
    }
 
    void accumulate_entries_into_not_investigated(std::vector<label_count_sum_tuple>& action_counter,
                            const std::vector<block_index>& todo_stack)
    {
      transition_index sum=0;
      for(block_index index: todo_stack)
      {
        action_counter[index].not_investigated=sum;
        sum=sum+action_counter[index].label_counter;
      }
    }

    /* void accumulate_entries(std::vector<label_count_sum_triple>& action_counter,
                            const std::vector<label_index>& todo_stack)
    {
      transition_index sum=0;
      for(label_index index: todo_stack)
      {
        action_counter[index].cumulative_label_counter=sum;
        action_counter[index].not_investigated=sum;
        sum=sum+action_counter[index].label_counter;
      }
    } */

    void accumulate_entries(std::vector<transition_index>& counter)
    {
      transition_index sum=0;
      for(transition_index& index: counter)
      {
        transition_index n=index;
        index=sum;
        sum=sum+n;
      }
    }

    void reset_entries(std::vector<label_count_sum_tuple>& action_counter,
                       std::vector<block_index>& todo_stack)
    {
      for(block_index index: todo_stack)
      {
        // it is not necessary to reset the cumulative_label_counter;
        action_counter[index].label_counter=0;
      }
      todo_stack.clear();
    }

    transition_index accumulate_entries(std::vector<transition_index>& action_counter,
                                   const std::vector<label_index>& todo_stack) const
    { 
      transition_index sum=0;
      for(label_index index: todo_stack)
      {
        transition_index n=sum;
        sum=sum+action_counter[index];
        action_counter[index]=n;
      }
      return sum;
    }

    // Group the elements from begin up to end, using a range from [0,number of blocks),
    // where each transition pinpointed by the iterator has as value its source block.
    
    void group_in_situ(const std::vector<transition_index>::iterator begin,
                       const std::vector<transition_index>::iterator end,
                       std::vector<block_index>& todo_stack,
                       std::vector<label_count_sum_tuple>& value_counter)
    {
      // Initialise the action counter.
      // The work in reset_entries() can be subsumed under mCRL2complexity calls
      // that have been issued in earlier executions of group_in_situ().
      reset_entries(value_counter, todo_stack);
      #ifdef CHECK_COMPLEXITY_GJ
        const constellation_index new_constellation = m_constellations.size()-1;
        const unsigned max_C = check_complexity::log_n - check_complexity::ilog2(number_of_states_in_constellation(new_constellation));
      #endif
      for(std::vector<transition_index>::iterator i=begin; i!=end; ++i)
      {
        const transition& t = m_aut.get_transitions()[*i];
        #ifdef CHECK_COMPLEXITY_GJ
          assert(m_blocks[m_states[t.to()].block].constellation == new_constellation);
          mCRL2complexity_gj(&m_transitions[*i], add_work(check_complexity::group_in_situ__count_transitions_per_block, max_C), *this);
        #endif
        const block_index n=m_states[t.from()].block;

        // mCRL2complexity_gj(..., add_work(..., ...), *this);
        if (value_counter[n].label_counter==0)
        {
          todo_stack.push_back(n);
        }
        value_counter[n].label_counter++;
      }

      // The work in accumulate_entries_into_not_investigated() can be subsumed
      // under the above call to mCRL2complexity, because an entry in todo_stack
      // is only made if there is at least one transition from that block.
      accumulate_entries_into_not_investigated(value_counter, todo_stack);

      std::vector<block_index>::iterator current_value=todo_stack.begin();
      for(std::vector<transition_index>::iterator i=begin; i!=end; )
      {
        mCRL2complexity_gj(&m_transitions[*i], add_work(check_complexity::group_in_situ__swap_transition, max_C), *this);
        block_index n=m_states[m_aut.get_transitions()[*i].from()].block;
        if (n==*current_value)
        {
          value_counter[n].label_counter--;
          value_counter[n].not_investigated++;
          ++i;
          while (assert(current_value!=todo_stack.end()), value_counter[n].label_counter==0)
          {
            #ifdef CHECK_COMPLEXITY_GJ
              // This work needs to be assigned to some transition from block n.
              // Just to make sure we assign it to all such transitions:
              std::vector<transition_index>::iterator work_i = i;
              assert(begin != work_i);
              --work_i;
              assert(m_states[m_aut.get_transitions()[*work_i].from()].block == n);
              do
              {
                mCRL2complexity_gj(&m_transitions[*work_i], add_work(check_complexity::group_in_situ__skip_to_next_block, max_C), *this);
              }
              while (begin != work_i && m_states[m_aut.get_transitions()[*--work_i].from()].block == n);
            #endif
            current_value++;
            if (current_value!=todo_stack.end())
            {
              n = *current_value;
              i=begin+value_counter[n].not_investigated; // Jump to the first non investigated action.
            }
            else
            {
              assert(i == end);
              break; // exit the while and the for loop.
            }
          }
        }
        else
        {
          // Find the first transition with a different label than t.label to swap with. 
          std::vector<transition_index>::iterator new_position=begin+value_counter[n].not_investigated;
          while (m_states[m_aut.get_transitions()[*new_position].from()].block==n)
          {
            mCRL2complexity_gj(&m_transitions[*new_position], add_work(check_complexity::group_in_situ__swap_transition, max_C), *this);
            value_counter[n].not_investigated++;
            value_counter[n].label_counter--;
            new_position++;
            assert(new_position!=end);
          }
          assert(value_counter[n].label_counter>0);
          std::swap(*i, *new_position);
          value_counter[n].not_investigated++;
          value_counter[n].label_counter--;
        }
      }
    }

//================================================= Create initial partition ========================================================
    void create_initial_partition()
    {
      mCRL2log(log::verbose) << "An O(m log n) "
           << (m_branching ? (m_preserve_divergence
                                         ? "divergence-preserving branching "
                                         : "branching ")
                         : "")
           << "bisimulation partitioner created for " << m_aut.num_states()
           << " states and " << m_aut.num_transitions() << " transitions (using the experimental algorithm GJ2024).\n";
      // Initialisation.
      #ifdef CHECK_COMPLEXITY_GJ
        check_complexity::init(2 * m_aut.num_states()); // we need ``2*'' because there is one additional call to splitB during initialisation
      #endif
    
      // Initialise m_incoming_(non-)inert-transitions, m_outgoing_transitions, and m_states[si].no_of_outgoing_transitions 
      //group_transitions_on_label(m_aut.get_transitions(), 
      //                          [](const transition& t){ return t.label(); }, 
      //                          m_aut.num_action_labels(), m_aut.tau_label_index()); // sort on label. Tau transitions come first.
      // group_transitions_on_label(m_aut.get_transitions(), 
      //                           [](const transition& t){ return t.from(); }, 
      //                           m_aut.num_states(), 0); // sort on label. Tau transitions come first.
      // group_transitions_on_label_tgt(m_aut.get_transitions(), m_aut.num_action_labels(), m_aut.tau_label_index(), m_aut.num_states()); // sort on label. Tau transitions come first.
      // group_transitions_on_tgt(m_aut.get_transitions(), m_aut.num_action_labels(), m_aut.tau_label_index(), m_aut.num_states()); // sort on label. Tau transitions come first.
      // sort_transitions(m_aut.get_transitions(), lbl_tgt_src);
// David suggests: I think it is enough to sort according to tgt_lbl.
      sort_transitions(m_aut.get_transitions(), m_aut.hidden_label_set(), tgt_lbl_src); // THIS IS NOW ESSENTIAL.
      // sort_transitions(m_aut.get_transitions(), src_lbl_tgt);
      // sort_transitions(m_aut.get_transitions(), tgt_lbl);
      // sort_transitions(m_aut.get_transitions(), target);

mCRL2log(log::verbose) << "Start setting incoming and outgoing transitions\n";

      // Count the number of occurring action labels. 
      assert((unsigned) m_preserve_divergence <= 1);
      std::vector<transition_index> count_transitions_per_action(m_aut.num_action_labels() + (unsigned) m_preserve_divergence, 0);
// David suggests: The above allocation may take time up to O(|Act|).
// This is a place where the number of actions plays a role.

      // David has changed the code to: if we have to preserve divergence, tau-self-loops get the artificial label m_aut.num_action_labels().
      std::vector<label_index> todo_stack_actions;
      if (m_branching)
      {
        todo_stack_actions.push_back(m_aut.tau_label_index()); // ensure that inert transitions come first
        count_transitions_per_action[m_aut.tau_label_index()] = 1; // set the number of transitions to a nonzero value so it doesn't trigger todo_stack_actions.push_back(...) in the loop
      }
      for(transition_index ti=0; ti<m_aut.num_transitions(); ++ti)
      {
        const transition& t=m_aut.get_transitions()[ti];
//std::cerr << "TRANS " << ptr(ti) << "\n";
        // mCRL2complexity_gj(&m_transitions[ti], add_work(..., 1), *this);
            // Because every transition is touched exactly once, we do not store a physical counter for this.
        label_index label = m_preserve_divergence && t.from() == t.to() && m_aut.is_tau(m_aut.apply_hidden_label_map(t.label()))
                            ? m_aut.num_action_labels() : m_aut.apply_hidden_label_map(t.label());
        transition_index& c=count_transitions_per_action[label];
        if (c==0)
        {
          todo_stack_actions.push_back(label);
        }
        c++;
      }
      if (m_branching)
      {
        --count_transitions_per_action[m_aut.tau_label_index()];
      }
//std::cerr << "COUNT_TRANSITIONS PER ACT1    "; for(auto s: count_transitions_per_action){ std::cerr << s << "  "; } std::cerr << "\n";
      accumulate_entries(count_transitions_per_action, todo_stack_actions);
//std::cerr << "COUNT_TRANSITIONS PER ACT2    "; for(auto s: count_transitions_per_action){ std::cerr << s << "  "; } std::cerr << "\n";
      std::vector<transition_index> transitions_per_action_label(m_aut.num_transitions());
      for(transition_index ti=0; ti<m_aut.num_transitions(); ++ti)
      {
        // mCRL2complexity_gj(&m_transitions[ti], add_work(..., 1), *this);
            // Because every transition is touched exactly once, we do not store a physical counter for this.
        const transition& t=m_aut.get_transitions()[ti];
        label_index label = m_preserve_divergence && t.from() == t.to() && m_aut.is_tau(m_aut.apply_hidden_label_map(t.label()))
                            ? m_aut.num_action_labels() : m_aut.apply_hidden_label_map(t.label());
        transition_index& c=count_transitions_per_action[label];
        transitions_per_action_label[c]=ti; // insert, if it does not occur. 
        c++;
      }
mCRL2log(log::verbose) << "Grouped transitions per action. \n";
      std::vector<transition_index> count_outgoing_transitions_per_state(m_aut.num_states(), 0);
      std::vector<transition_index> count_incoming_transitions_per_state(m_aut.num_states(), 0);

      // Group transitions per outgoing/incoming state.
      for(const transition& t: m_aut.get_transitions())
      { 
        // mCRL2complexity_gj(&m_transitions[std::distance(&*m_aut.get_transitions.begin(), &t)], add_work(..., 1), *this);
            // Because every transition is touched exactly once, we do not store a physical counter for this.
        count_outgoing_transitions_per_state[t.from()]++;
        count_incoming_transitions_per_state[t.to()]++;
        if (is_inert_during_init(t))
        {
          m_states[t.from()].no_of_outgoing_inert_transitions++;
        }
      }   
  
      m_outgoing_transitions.resize(m_aut.num_transitions());
      // We now set the outgoing transition per state pointer to the first non-inert transition.
      // The counters for outgoing transitions calculated above are reset to 0
      // and will later contain the number of transitions already stored.
      // Every time an inert transition is stored, the outgoing transition per state pointer is reduced by one.
      outgoing_transitions_it current_outgoing_transitions = m_outgoing_transitions.begin();

      // place transitions and set the pointers to incoming/outgoing transitions
      for (state_index s = 0; s < m_aut.num_states(); ++s)
      {
        // mCRL2complexity_gj(&m_states[s], add_work(..., 1), *this);
            // Because every state is touched exactly once, we do not store a physical counter for this.
        m_states[s].start_outgoing_transitions = current_outgoing_transitions + m_states[s].no_of_outgoing_inert_transitions;
        current_outgoing_transitions += count_outgoing_transitions_per_state[s];
        count_outgoing_transitions_per_state[s] = 0; // meaning of this counter changes to: number of outgoing transitions already stored
      }
      assert(current_outgoing_transitions == m_outgoing_transitions.end());

mCRL2log(log::verbose) << "Moving incoming and outgoing transitions\n";

      for(const transition_index ti: transitions_per_action_label)
      // std::size_t position=0;
      // for(const transition& t: m_aut.get_transitions())
      {
        // mCRL2complexity_gj(&m_transitions[ti], add_work(..., 1), *this);
            // Because every transition is touched exactly once, we do not store a physical counter for this.
        const transition& t=m_aut.get_transitions()[ti];
        // David has changed this: the original code could not accommodate correctly
        // for tau-self-loops in dpbranching-bisim-gj mode, as the tau-self-loop
        // is a non-inert transition that may be found between inert transitions.
        if (is_inert_during_init(t))
        {
          m_transitions[ti].ref_outgoing_transitions = --m_states[t.from()].start_outgoing_transitions;
        }
        else
        {
          m_transitions[ti].ref_outgoing_transitions = m_states[t.from()].start_outgoing_transitions + count_outgoing_transitions_per_state[t.from()];
        }
        m_transitions[ti].ref_outgoing_transitions->transition = ti;  
        ++count_outgoing_transitions_per_state[t.from()];
      }

      // Set start_incoming_non_inert_transition for each state.
      /* state_index current_state=null_state;
      bool tau_transitions_passed=true;
      for( std::vector<transition_index>::iterator it=m_incoming_transitions.begin(); it!=m_incoming_transitions.end(); it++)
      {
        const transition& t=m_aut.get_transitions()[*it];
        if (t.to()==current_state)
        { 
          if (!m_aut.is_tau(m_aut.apply_hidden_label_map(t.label())) && !tau_transitions_passed)
          {
            tau_transitions_passed=true;
            m_states[current_state].start_incoming_non_inert_transitions=it;
          }
        }
        else
        {
          if (!tau_transitions_passed)
          {
            m_states[current_state].start_incoming_non_inert_transitions=it;
          }
          current_state=t.to();
          if (m_aut.is_tau(m_aut.apply_hidden_label_map(t.label())))
          {
            tau_transitions_passed=false;
          }
          else
          {
            tau_transitions_passed=true;
            m_states[current_state].start_incoming_non_inert_transitions=it;
          }
        }
      }
      if (!tau_transitions_passed)
      {
        m_states[current_state].start_incoming_non_inert_transitions=m_incoming_transitions.end();
      } */
      state_index current_state=null_state;
      assert(current_state + 1 == 0);
      // bool tau_transitions_passed=true;
      // TODO: This should be combined with another pass through all transitions. 
      for(std::vector<transition>::iterator it=m_aut.get_transitions().begin(); it!=m_aut.get_transitions().end(); it++)
      {
        // mCRL2complexity_gj(&m_transitions[std::distance(m_aut.get_transitions().begin(), it)], add_work(..., 1), *this);
            // Because every transition is touched exactly once, we do not store a physical counter for this.
        const transition& t=*it;
        if (t.to()!=current_state)
        { 
          for (state_index i=current_state+1; i<=t.to(); ++i)
          {
            // ensure that every state is visited at most once:
            mCRL2complexity_gj(&m_states[i], add_work(check_complexity::create_initial_partition__set_start_incoming_transitions, 1), *this);
//std::cerr << "SET start_incoming_transitions for state " << i << "\n";
            m_states[i].start_incoming_transitions=it;
          }
          current_state=t.to();
        }
      }
      for (state_index i=current_state+1; i<m_states.size(); ++i)
      {
        mCRL2complexity_gj(&m_states[i], add_work(check_complexity::create_initial_partition__set_start_incoming_transitions, 1), *this);
//std::cerr << "SET residual start_incoming_transitions for state " << i << "\n";
        m_states[i].start_incoming_transitions=m_aut.get_transitions().end();
      }

      // Set the start_same_saC fields in m_outgoing_transitions.
      outgoing_transitions_reverse_it it = m_outgoing_transitions.rbegin();
      if (it != m_outgoing_transitions.rend())
      {
        const transition& t = m_aut.get_transitions()[it->transition];
        state_index current_state = t.from();
        label_index current_label = m_preserve_divergence && t.from() == t.to() && m_aut.is_tau(m_aut.apply_hidden_label_map(t.label()))
                                    ? m_aut.num_action_labels() : m_aut.apply_hidden_label_map(t.label());
        outgoing_transitions_it current_start_same_saC = (it+1).base();
        while (++it != m_outgoing_transitions.rend())
        {
          // mCRL2complexity_gj(&m_transitions[it->transition], add_work(..., 1), *this);
              // Because every transition is touched exactly once, we do not store a physical counter for this.
          const transition& t = m_aut.get_transitions()[it->transition];
          const label_index new_label = m_preserve_divergence && t.from() == t.to() && m_aut.is_tau(m_aut.apply_hidden_label_map(t.label()))
                                        ? m_aut.num_action_labels() : m_aut.apply_hidden_label_map(t.label());
          if (current_state == t.from() && current_label == new_label)
          {
            // We encounter a transition with the same saC. Let it refer to the end.
            it->start_same_saC = current_start_same_saC;
          }
          else
          {
            // We encounter a transition with a different saC.
            current_state = t.from();
            current_label = new_label;
            current_start_same_saC->start_same_saC=it.base();  // This refers to position it+1;
            current_start_same_saC = (it+1).base();
          }
        }
        current_start_same_saC->start_same_saC=m_outgoing_transitions.begin();
      }

//std::cerr << "Start filling states_in_blocks\n";
      m_states_in_blocks.resize(m_aut.num_states());
      typename std::vector<state_index>::iterator lower_i=m_states_in_blocks.begin(), upper_i=m_states_in_blocks.end();
      for (state_index i=0; i < m_aut.num_states(); ++i)
      {
        // mCRL2complexity_gj(&m_states[i], add_work(..., 1), *this);
            // Because every state is touched exactly once, we do not store a physical counter for this.
        if (0 < m_states[i].no_of_outgoing_inert_transitions)
        {
          --upper_i;
          *upper_i = i;
          m_states[i].ref_states_in_blocks = upper_i;
        }
        else
        {
          *lower_i = i;
          m_states[i].ref_states_in_blocks = lower_i;
          ++lower_i;
        }
      }
      assert(lower_i == upper_i);
      m_blocks[0].start_bottom_states=m_states_in_blocks.begin();
      m_blocks[0].start_non_bottom_states = lower_i;
      m_blocks[0].end_states=m_states_in_blocks.end();

      // print_data_structures("After initial reading before splitting in the initialisation",true);
      assert(check_data_structures("After initial reading before splitting in the initialisation",true));

mCRL2log(log::verbose) << "Start refining in the initialisation\n";

//std::cerr << "COUNT_STATES PER ACT     "; for(auto s: count_transitions_per_action){ std::cerr << s << "  "; } std::cerr << "\n";
//std::cerr << "STATES PER ACTION LABEL  "; for(auto s: transitions_per_action_label){ std::cerr << s << "  "; } std::cerr << "\n";
//std::cerr << "STATES PER ACTION LABELB "; for(auto s: transitions_per_action_label){ std::cerr << m_states[m_aut.get_transitions()[s].from()].block << "  "; } std::cerr << "\n";
      // Now traverse the states per equal label, grouping them first on outgoing block.
      std::vector<transition_index>::iterator start_index=transitions_per_action_label.begin();
      std::vector<label_count_sum_tuple> value_counter(m_blocks.size());
      std::vector<block_index> todo_stack_blocks;
      
     
      for(label_index a: todo_stack_actions)
      {
// mCRL2log(log::debug) << "--------------------------------------------------\n";
// mCRL2log(log::debug) << "CONSIDER ACTION ";
// if (m_preserve_divergence && m_aut.num_action_labels() == a) { mCRL2log(log::debug) << "(tau-self-loops)   "; } else { mCRL2log(log::debug) << m_aut.action_label(a) << "   "; }
// mCRL2log(log::debug) << count_transitions_per_action[a] << "\n";
          // mCRL2complexity_gj(...)
              // not needed as the inner loop is always executed at least once.
          std::vector<transition_index>::iterator end_index=transitions_per_action_label.begin()+count_transitions_per_action[a];
          if (!m_branching || !m_aut.is_tau(a)) // skip inert transitions
          {
            // Group the states per block.
            value_counter.resize(m_blocks.size());
            assert(todo_stack_blocks.empty());
// mCRL2log(log::debug) << "INDICES " << std::distance(start_index, end_index) << "   " << &*start_index << "    " << &*end_index << "\n";
            group_in_situ(start_index,
                          end_index,
                          todo_stack_blocks,
                          value_counter);
            std::vector<transition_index>::iterator start_index_per_block=start_index;
            assert(!todo_stack_blocks.empty()); // ensure that the inner loop is executed
            for(block_index block_ind: todo_stack_blocks)
            {
// mCRL2log(log::debug) << "INVESTIGATED " << block_ind << "    " << value_counter[block_ind].not_investigated << "\n";
              std::vector<transition_index>::iterator end_index_per_block=start_index+value_counter[block_ind].not_investigated;
// mCRL2log(log::debug) << "RANGE " << &*start_index_per_block << "    " << &*end_index_per_block << "\n";
// mCRL2log(log::debug) << "TRANSITIONS PER ACTION LABEL  "; for(auto s: transitions_per_action_label){ mCRL2log(log::debug) << s << "  "; } mCRL2log(log::debug) << "\n";
              #ifdef CHECK_COMPLEXITY_GJ
                // We have to assign the work to some transition in [start_index_per_block, end_index_per_block).
                // In fact, just to make sure, we assign it to every transition.
                assert(start_index_per_block!=end_index_per_block);
                std::vector<transition_index>::iterator work_i = start_index_per_block;
                do
                {
                  mCRL2complexity_gj(&m_transitions[*work_i], add_work(check_complexity::create_initial_partition__select_action_label_and_block_to_be_split, 1), *this);
                }
                while (++work_i!=end_index_per_block);
              #endif
              // Check whether the block B, indexed by block_ind, can be split.
              // This means that the bottom states of B are not all in the split_states.
              const block_type& B=m_blocks[block_ind];

              // Count the number of state in split_states that are bottom states. This function has a side effect if not all bottom states are covered
              // on m_P and counters in states. 
              if (not_all_bottom_states_are_touched
                      (block_ind, start_index_per_block, end_index_per_block))
              { 
                bool dummy=false;
                // std::size_t dummy_number=-1;
                const bool initialisation=true;
                splitB<1>(block_ind, 
                          start_index_per_block,
                          end_index_per_block,
                          B.start_bottom_states,
                          B.start_non_bottom_states,
                          a,   // action index (needed to account for the work)
                          0,   // constellation index (there is only one constellation, with number 0)
                          dummy,
                          [](const block_index, const block_index, const transition_index, const transition_index){},
                          initialisation);
              }
              start_index_per_block=end_index_per_block;
            }
            todo_stack_blocks.clear();
          }
          start_index=end_index;
      }
     
      // The initial partition has been constructed. Continue with the initiatialisation.
      // Initialise m_transitions[...].transitions_per_block_to_constellation. .

mCRL2log(log::verbose) << "Start post-refinement initialisation of the LBC list in the initialisation\n";
      // Move all transitions to m_BLC_transitions and sort on actions afterwards;
      std::vector<transition_index> count_transitions_per_block(m_blocks.size(),0);
      for(const transition& t: m_aut.get_transitions())
      {
        // mCRL2complexity_gj(&m_transitions[std::distance(&*m_aut.gt_transitions().begin(), &t)], add_work(..., 1), *this);
            // Because every transition is touched exactly once, we do not store a physical counter for this.
        count_transitions_per_block[m_states[t.from()].block]++;
      }
      accumulate_entries(count_transitions_per_block);

      for(const transition_index ti: transitions_per_action_label)
      // position=0;
      // for(const transition& t: m_aut.get_transitions())
      {
        // mCRL2complexity_gj(&m_transitions[ti], add_work(..., 1), *this);
            // Because every transition is touched exactly once, we do not store a physical counter for this.
        const transition& t=m_aut.get_transitions()[ti];
        transition_index& pos=count_transitions_per_block[m_states[t.from()].block];
        // m_BLC_transitions[pos]=position;
        m_BLC_transitions[pos]=ti;
//std::cerr << "INSERT BLC Trans  " << count_transitions_per_block[m_states[t.from()].block] << "\n";
        pos++;
        // position++;
      } 

      block_index current_block=null_block;
      label_index current_label=null_action;
      transition_index current_start=0;
      bool current_transition_is_selfloop = false;
      typename linked_list<BLC_indicators>::iterator new_position;
      for(std::vector<transition_index>::iterator ti=m_BLC_transitions.begin(); ti!=m_BLC_transitions.end(); ++ti)
      {
        // mCRL2complexity_gj(&m_transitions[*ti], add_work(..., 1), *this);
            // Because every transition is touched exactly once, we do not store a physical counter for this.
        const transition& t=m_aut.get_transitions()[*ti];

        transition_index current_position=std::distance(m_BLC_transitions.begin(),ti);
        if (m_aut.apply_hidden_label_map(t.label())!=current_label || m_states[t.from()].block!=current_block ||
                (m_preserve_divergence && m_aut.is_tau(current_label) && (t.from() == t.to()) != current_transition_is_selfloop))
        {
          if (current_label!=null_action)
          {
//std::cerr << "PUSH BACK FRONT " << current_start << "    " << current_position << "\n";
            block_type& b=m_blocks[current_block];
            new_position=b.block_to_constellation.emplace_after(b.block_to_constellation.begin(),
                                                                m_BLC_transitions.begin()+current_start, 
                                                                m_BLC_transitions.begin()+current_position);
            for(std::vector<transition_index>::iterator tti=m_BLC_transitions.begin()+current_start; tti!=m_BLC_transitions.begin()+current_position; ++tti)
            {
              mCRL2complexity_gj(&m_transitions[*tti], add_work(check_complexity::create_initial_partition__set_transitions_per_block_to_constellation, 1), *this);
              m_transitions[*tti].transitions_per_block_to_constellation=new_position;
            }
          }
          current_block=m_states[t.from()].block;
          current_label=m_aut.apply_hidden_label_map(t.label());
          current_transition_is_selfloop = t.from() == t.to();
          current_start=current_position;
        }

        // m_transitions[*ti].ref_BLC_list=ti;
        m_transitions[*ti].ref_outgoing_transitions->transition = current_position;
      }
      if (current_label!=null_action)
      {
        block_type& b=m_blocks[current_block];
        new_position=b.block_to_constellation.emplace_after(b.block_to_constellation.begin(),
                                                            m_BLC_transitions.begin()+current_start, 
                                                            m_BLC_transitions.end());
        for(std::vector<transition_index>::iterator tti=m_BLC_transitions.begin()+current_start; tti!=m_BLC_transitions.end(); ++tti)
        {
          mCRL2complexity_gj(&m_transitions[*tti], add_work(check_complexity::create_initial_partition__set_transitions_per_block_to_constellation, 1), *this);
          m_transitions[*tti].transitions_per_block_to_constellation=new_position;
        }
      }

      // The data structures have now been completely initialized.

      // Algorithm 1, line 1.4 is implicitly done in the call to splitB above.
      
      // Algorithm 1, line 1.5.
      // print_data_structures("End initialisation");
mCRL2log(log::verbose) << "Start stabilizing in the initialisation\n";
      assert(check_data_structures("End initialisation"));
      stabilizeB();
    }
 
    // Algorithm 4. Stabilize the current partition with respect to the current constellation
    // given that the states in m_P did become new bottom states. 
    // Stabilisation is always called after initialisation, i.e., m_incoming_transitions[ti].transition refers
    // to a position in m_BLC_transitions, where the transition index of this transition can be found.

    void stabilizeB()
    {
      // Algorithm 4, line 4.3.
      std::unordered_map<block_index, set_of_states_type> Phat;
      for(const state_index si: m_P)
      {
        Phat[m_states[si].block].emplace(si);
      }
      clear(m_P);

      // Algorithm 4, line 4.4.
      while (!Phat.empty()) 
      {
        // Algorithm 4, line 4.5. 
        const block_index bi=Phat.begin()->first;
        const set_of_states_type& V=Phat.begin()->second;
        
        // Algorithm 4, line 4.6.
        // Collect all new bottom states and group them per action-constellation pair.
        label_constellation_to_set_of_states_map grouped_transitions;
        for(const state_index si: V)
        {
          const outgoing_transitions_it end_it=(si+1>=m_states.size())?m_outgoing_transitions.end():m_states[si+1].start_outgoing_transitions;
          for(outgoing_transitions_it ti=m_states[si].start_outgoing_transitions;
                        ti!=end_it;
                    ++ti)
          {
            transition& t=m_aut.get_transitions()[m_BLC_transitions[ti->transition]];
            if (!(is_inert_during_init(t) && m_states[t.from()].block==m_states[t.to()].block))
            {
              grouped_transitions[std::pair(t.label(), m_blocks[m_states[t.to()].block].constellation)].emplace(t.from());
            }
          }
        }

        std::unordered_map<block_index, set_of_states_type> Ptilde;
        assert(!V.empty());
        Ptilde[bi]=V;
        
        // Algorithm 4, line 4.7.
        typedef std::unordered_map< std::pair < block_index, std::pair<label_index, constellation_index > >, transition_index > Qhat_map;
        Qhat_map Qhat;
        for(linked_list< BLC_indicators >::iterator blc_it=m_blocks[bi].block_to_constellation.begin();
                                   blc_it!=m_blocks[bi].block_to_constellation.end(); ++blc_it)
        { transition_index ti=*(blc_it->start_same_BLC);
          const transition& t=m_aut.get_transitions()[ti];
          if (!is_inert_during_init(t) || m_blocks[m_states[t.to()].block].constellation!=m_blocks[bi].constellation)
          {
            Qhat[std::pair(bi, std::pair(t.label(), m_blocks[m_states[t.to()].block].constellation))]=ti;
          }
        }
  
        
        // Algorithm 4, line 4.8.
        while (!Qhat.empty())
        {
          // Algorithm 4, line 4.9.
          const typename Qhat_map::iterator Qit=Qhat.begin();
          const transition_index t_ind=Qit->second;
          const transition& t=m_aut.get_transitions()[t_ind];
//std::cerr << "Indicating transition  " << t.from() << " -" << m_aut.action_label(t.label()) << "-> " << t.to() << "\n";
          Qhat.erase(Qit);
          // Algorithm 4, line 4.10.
          const block_index bi=m_states[t.from()].block;
          set_of_states_type W=Ptilde[bi]; //TODO: Should be a reference?
          const set_of_states_type& aux=grouped_transitions[std::pair(t.label(), m_blocks[m_states[t.to()].block].constellation)];
          /* bool W_empty=true;
//std::cerr << "W: "; for(auto s: W) { std::cerr << s << " "; } std::cerr << "\n";
          for(const state_index si: W) 
          {
            if (aux.count(si)==0) 
            {
              W_empty=false;
              break;
            }
          } */
          // Algorithm 4, line 4.10.
          if (!W_empty(W, aux))
          {
// mCRL2log(log::debug) << "PERFORM A NEW BOTTOM STATE SPLIT\n";
            // Algorithm 4, line 4.11, and implicitly 4.12, 4.13 and 4.18. 
            bool V_in_bi=false;
            splitB<2>(bi, 
                   m_transitions[t_ind].transitions_per_block_to_constellation->start_same_BLC,
                   m_transitions[t_ind].transitions_per_block_to_constellation->end_same_BLC,
                   W.begin(), 
                   W.end(),
                   t.label(),
                   m_blocks[m_states[t.to()].block].constellation,
                   V_in_bi,
                   [](const block_index, const block_index, const transition_index, const transition_index){},
                   false,  // Update LBC-list.
            // Algorithm 4, line 4.14 and 4.15.
                   [&Qhat, this]
                   (const transition_index ti, const transition_index remaining_transition, const block_index old_block)
                     { 
                       // Move the new bottom state to its appropriate block.
                       const transition& t=m_aut.get_transitions()[ti];
 
                       // Check whether this transition represented a label/constellation in the old block. 
                       typename Qhat_map::iterator Qti_it=Qhat.find(std::pair(old_block, std::pair(t.label(),m_blocks[m_states[t.to()].block].constellation)));
                       if (Qti_it!=Qhat.end())  
                       {
                         // This transition is not investigated, or was not inert for the target constellation, and did not need to be investigated.
                         if (Qti_it->second==ti)
                         {
                           // This transition was used a representation. Remove it, and Insert the alternative transition. 
                           if (remaining_transition==null_transition)
                           {
                             Qhat.erase(Qti_it);
                           }
                           else
                           {
                             Qti_it->second=remaining_transition;
                           }
                         }
                       
                         // Check whether for the new block this transition is to be used as a representation for an outgoing label/constellation. 
                         Qti_it=Qhat.find(std::pair(m_states[t.from()].block, std::pair(t.label(),m_blocks[m_states[t.to()].block].constellation)));
                         if (Qti_it==Qhat.end())
                         {
                           // Insert this transition as a representation for an outgoing label/constellation pair.
                           Qhat[std::pair(m_states[t.from()].block, std::pair(t.label(),m_blocks[m_states[t.to()].block].constellation))]=ti;
                         }
                       }
                     },
                     // Algorithm 4, line 4.16 and 4.17.
                    [&Ptilde, bi, this](const state_index si)
                       {
                         const typename std::unordered_map<block_index, set_of_states_type>::iterator iter=Ptilde.find(bi);
                         if (iter!=Ptilde.end())
                         {
                           if (iter->second.count(si)>0) // if si is a new bottom state, move it. 
                           {  
                             assert(bi!=m_states[si].block);
                             iter->second.erase(si);
                             if (iter->second.empty())
                             { 
                               // Ptilde.erase(bi);
                               Ptilde.erase(iter);
                             }
                             Ptilde[m_states[si].block].emplace(si);
                           }
                         }
                       });

          }
        }
        Phat.erase(bi);
        
        // Algorithm 4, line 4.17.
        for(const state_index si: m_P)
        {
          Phat[m_states[si].block].emplace(si);
        }
        clear(m_P);

      }
    }

    void maintain_block_label_to_cotransition(
                   const block_index old_block,
                   const block_index new_block,
                   const transition_index moved_transition,
                   const transition_index alternative_transition,
                   block_label_to_size_t_map& block_label_to_cotransition,
                   const constellation_index ci) const
    {
      const transition& t_move=m_aut.get_transitions()[moved_transition];
      if (m_blocks[m_states[t_move.to()].block].constellation==ci &&
          (!is_inert_during_init(t_move) || m_blocks[m_states[t_move.from()].block].constellation!=ci))
      { 
        // This is a transition to the current co-constellation.
        
        typename block_label_to_size_t_map::const_iterator bltc_it=
                       block_label_to_cotransition.find(std::pair(old_block,t_move.label()));
        if (bltc_it!=block_label_to_cotransition.end())
        {
          if (bltc_it->second==moved_transition)
          {
            // This transition is being moved. Find a replacement in block_label_to_cotransition.
            block_label_to_cotransition[std::pair(old_block,t_move.label())]=alternative_transition;
          }
        }

        // Check whether there is a representation for the new_block in block_label_to_cotransition.
        bltc_it=block_label_to_cotransition.find(std::pair(new_block,t_move.label()));
        if (bltc_it==block_label_to_cotransition.end())
        {
          // No such transition exists as yet. Give moved transition this purpose.
          block_label_to_cotransition[std::pair(new_block,t_move.label())]=moved_transition;
        }
      }
    }

    transition_index find_inert_co_transition_for_block(const block_index index_block_B, const constellation_index old_constellation)
    {
      transition_index co_t=null_transition;
      linked_list< BLC_indicators >::iterator btc_it= m_blocks[index_block_B].block_to_constellation.begin();
      if (btc_it!=m_blocks[index_block_B].block_to_constellation.end())
      {  
        const transition& btc_t=m_aut.get_transitions()[*(btc_it->start_same_BLC)];
        if (is_inert_during_init(btc_t) && m_blocks[m_states[btc_t.to()].block].constellation==old_constellation)
        {
          co_t=*(btc_it->start_same_BLC);
        }
        else 
        {
          ++btc_it;
          if (btc_it!=m_blocks[index_block_B].block_to_constellation.end())
          { 
            const transition& btc_t=m_aut.get_transitions()[*(btc_it->start_same_BLC)];
            if (is_inert_during_init(btc_t) && m_blocks[m_states[btc_t.to()].block].constellation==old_constellation)
            {
              co_t=*(btc_it->start_same_BLC);
            }
          }
        } 
      } 
      return co_t;
    }


    // This routine can only be called after initialisation, i.e., when m_outgoing_transitions[t].transition refers to 
    // a position in m_BLC_transitions.
    bool state_has_outgoing_co_transition(const transition_index transition_to_bi, const constellation_index old_constellation)
    {
      // i1 refers to the position of the transition_to_bi;
      outgoing_transitions_it i1=m_transitions[transition_to_bi].ref_outgoing_transitions;
      // i2 refers to the last position, unless i1 is the last, then i2 is the first of all transitions with the same s, a and C.
      outgoing_transitions_it i2=i1->start_same_saC;
      if (i2<=i1) 
      {
        // So, i1 is the last element with the same s, a, C.
        i2=i1+1;
      }
      else
      {
        // In this case i2 refers to the last element with the same s, a, C.
        i2++;
      }
      if (i2==m_outgoing_transitions.end())
      {
        return false;
      }
      const transition& t1=m_aut.get_transitions()[transition_to_bi];
      const transition& t2=m_aut.get_transitions()[m_BLC_transitions[i2->transition]]; 
      return t1.from()==t2.from() && t1.label()==t2.label() && m_blocks[m_states[t2.to()].block].constellation==old_constellation;
    }

    // This function determines whether all bottom states in B have outgoing co-transitions. If yes false is reported.
    // If no, true is reported and the source states with outgoing co-transitions are added to m_R and those without outgoing
    // co-transitions are added to m_u. The counters of these states are set to Rmarked or to 0. This already initialises
    // these states for the splitting process. 
    // This routine is called after initialisation. 
    bool some_bottom_state_has_no_outgoing_co_transition(block_index B, 
                                                         std::vector<transition_index>::iterator transitions_begin,
                                                         std::vector<transition_index>::iterator transitions_end,
                                                         const constellation_index old_constellation)
    {
      state_index nr_of_touched_bottom_states=0;
      #ifdef CHECK_COMPLEXITY_GJ
        const constellation_index new_constellation = m_constellations.size()-1;
        const unsigned max_C = check_complexity::log_n - check_complexity::ilog2(number_of_states_in_constellation(new_constellation));
      #endif
      for(std::vector<transition_index>::iterator ti=transitions_begin; ti!=transitions_end; ++ti)
      {
        const transition& t=m_aut.get_transitions()[*ti];
//std::cerr << "INSPECT TRANSITION  " << t.from() << " -" << m_aut.action_label(t.label()) << "-> " << t.to() << "\n";
        #ifdef CHECK_COMPLEXITY_GJ
          assert(new_constellation==m_blocks[m_states[t.to()].block].constellation);
          mCRL2complexity_gj(&m_transitions[*ti], add_work(check_complexity::some_bottom_state_has_no_outgoing_co_transition__handle_transition, max_C), *this);
        #endif
        assert(m_states[t.from()].ref_states_in_blocks>=m_blocks[B].start_bottom_states);
        assert(m_states[t.from()].ref_states_in_blocks<m_blocks[B].end_states);
        if (m_states[t.from()].ref_states_in_blocks<m_blocks[B].start_non_bottom_states &&
            m_states[t.from()].counter==undefined)
        {
            if (state_has_outgoing_co_transition(*ti,old_constellation))
            {
              nr_of_touched_bottom_states++;
              m_states[t.from()].counter=Rmarked;
              m_R.add_todo(t.from());
            }
            else
            {
              // We need to register that we added this state to U
              // because it might have other transitions to new_constellation,
              // so it will be revisited
              m_U_counter_reset_vector.push_back(t.from());
              m_states[t.from()].counter=0;
              m_U.add_todo(t.from());

            }
        }
      }
      
      const state_index number_of_bottom_states=std::distance(m_blocks[B].start_bottom_states,m_blocks[B].start_non_bottom_states);
      if (number_of_bottom_states>nr_of_touched_bottom_states)
      {
        return true; // A split can commence. 
      }
      // Otherwise, reset the marks. 
      clear_state_counters();
      m_R.clear();
      m_U.clear();
      return false;

      /* for(std::vector<transition_index>::iterator ti=transitions_begin; ti!=transitions_end; ++ti)
      {
        const transition& t=m_aut.get_transitions()[*ti];
        if (m_states[t.from()].ref_states_in_blocks<m_blocks[B].start_non_bottom_states)
        {
          m_states[t.from()].counter=undefined;
        }
      } */
    }

    // Check if there is a state in W that has no outgoing a transition to some constellation.
    // If so, return false, but set in m_R and m_U whether those states in W have or have no
    // outgoing transitions. Set m_states[s].counter accordingly.
    // If all states in W have outgoing transitions with the label and constellation, leave
    // m_R, m_U, m_states[s].counters and m_U_counter_reset vector untouched. 
    bool W_empty(const set_of_states_type& W, const set_of_states_type& aux)
    {
      bool W_empty=true;
//std::cerr << "W: "; for(auto s: W) { std::cerr << s << " "; } std::cerr << "\n";
      for(const state_index si: W)
      {
        assert(0 == m_states[si].no_of_outgoing_inert_transitions);
        if (aux.count(si)==0)
        {
          W_empty=false;
          m_U.add_todo(si);
        }
        else 
        {
          m_states[si].counter=Rmarked;
          m_R.add_todo(si);
        }
      }
      if (!W_empty)
      {
        return false; // A split can commence. 
      }
      // Otherwise, reset the marks. 
      clear_state_counters();
      m_R.clear();
      m_U.clear();
      return true;
    }

    // Count the number of touched bottom states by outgoing transitions in Mleft.
    // If all bottom states are touched, reset markers and m_R. Otherwise, if not all
    // bottom states are touched, leave the touched bottom states in m_R and leave them
    // the markers in m_states[*].counter in place.
    bool not_all_bottom_states_are_touched(
                      const block_index bi, 
                      const std::vector<transition_index>::iterator begin,
                      const std::vector<transition_index>::iterator end)
    {
      state_index no_of_touched_bottom_states=0;
      const block_type& B=m_blocks[bi];
      for(std::vector<transition_index>::iterator i=begin; i!=end; ++i)
      {           
        const transition& t = m_aut.get_transitions()[*i];
        const state_index s=t.from();
        mCRL2complexity_gj(&m_transitions[*i], add_work(check_complexity::not_all_bottom_states_are_touched__mark_source_state, check_complexity::log_n - check_complexity::ilog2(number_of_states_in_constellation(m_blocks[m_states[t.to()].block].constellation))), *this);
        if (is_inert_during_init(t) && m_states[s].block == m_states[t.to()].block)
        {
          // This is actually an inert transition. Disregard it for deciding whether to split the block.
          continue;
        }
        if (m_states[s].counter!=Rmarked)
        { 
          if (m_states[s].ref_states_in_blocks<B.start_non_bottom_states)
          {
            no_of_touched_bottom_states++;
// David suggests: Instead of only counting the bottom states,
// one could immediately group the bottom states into those that go into R
// (and those that do not). Then, after not_all_bottom_states_are_touched(),
// one has already separated the bottom states, and the U-coroutine to separate
// the nonbottom states can more easily continue the work.
// However, I am not sure which of the possibilities is actually faster.
          }
          m_R.add_todo(s);
          m_states[s].counter=Rmarked;
          // m_counter_reset_vector.push_back(s);
        } 
      }     
// David suggests: Set all marked states to the front, such that the non marked states in U can be traversed
//                  without again checking. JFG: doubts whether this checking will have a lot of effect, given
//                  that the states are in the cache anyhow....     
      if (static_cast<std::ptrdiff_t>(no_of_touched_bottom_states)==std::distance(B.start_bottom_states, B.start_non_bottom_states))
      {
        // All bottom states are touched. No splitting is possible. Reset m_R, m_states[s].counter for s in m_R.
        clear_state_counters(false);
        /* for(const state_index s: m_counter_reset_vector)
        {           
          m_states[s].counter=undefined;
        } */    
        m_R.clear();
        // clear(m_counter_reset_vector);
        return false;
      }
      // Not all bottom states are touched. Splitting of block bi must commence. 
      return true;
    }

    // This routine can only be called after initialisation.  
    bool hatU_does_not_cover_B_bottom(const block_index index_block_B, const constellation_index old_constellation)
    {  
      mCRL2complexity_gj(&m_blocks[index_block_B], add_work(check_complexity::hatU_does_not_cover_B_bottom__handle_bottom_states_and_their_outgoing_transitions_in_splitter, check_complexity::log_n - check_complexity::ilog2(number_of_states_in_block(index_block_B))), *this);
      assert(m_branching);
      bool hatU_does_not_cover_B_bottom=false;
      for(typename std::vector<state_index>::iterator si=m_blocks[index_block_B].start_bottom_states;
                        si!=m_blocks[index_block_B].start_non_bottom_states; 
                      ++si)
      {
        // mCRL2complexity_gj(&m_states[*si], add_work(..., max_C), *this);
            // subsumed by the above call
        bool found=false;
        const outgoing_transitions_it end_it=((*si)+1>=m_states.size())?m_outgoing_transitions.end():m_states[(*si)+1].start_outgoing_transitions;
        for(outgoing_transitions_it tti=m_states[*si].start_outgoing_transitions;
                                     !found && tti!=end_it;
                                     ++tti)
        { 
          // mCRL2complexity_gj(&m_transitions[tti->transition], add_work(..., max_C), *this);
              // subsumed by the above call
          const transition& t=m_aut.get_transitions()[m_BLC_transitions[tti->transition]];
          assert(t.from() == *si);
          if (m_aut.is_tau(m_aut.apply_hidden_label_map(t.label())) && m_blocks[m_states[t.to()].block].constellation==old_constellation)
          {
            found =true;
          }
        }
        if (!found)
        {
          // This state has no constellation-inert tau transition to the old constellation.
          hatU_does_not_cover_B_bottom=true;
          m_U.add_todo(*si);
        }
        else
        {
          // The state *si has a tau transition to the old constellation that has just become constellation-non-inert.
          m_R.add_todo(*si);
          m_states[*si].counter=Rmarked;
        }
      } 
      if (hatU_does_not_cover_B_bottom)
      {
        // Splitting can commence.
        return true;
      }
      else
      {
        // Splitting is not possible. Reset the counter in m_states.
        m_U.clear();
        clear_state_counters(true);
        m_R.clear();
        return false;
      }
    }


    // Select a block that is not the largest block in the constellation. 
    // It is advantageous to select the smallest block. 
    // The constellation ci is returned. 
    block_index select_and_remove_a_block_in_a_non_trivial_constellation(constellation_index& ci)
    {
#define MINIMAL_CHECKING
#ifdef MINIMAL_CHECKING
      // Do the minimal checking, i.e., only check two blocks in a constellation. 
      // This is equivalent to the code below by setting search_limit to 2.
      //const set_of_constellations_type::const_iterator i=m_non_trivial_constellations.begin();
      //ci= *i;
      ci=m_non_trivial_constellations.back();
      std::forward_list<block_index>::iterator fl=m_constellations[ci].blocks.begin();
      block_index index_block_B=*fl;       // The first block.
      block_index second_block_B=*(++fl);  // The second block.
    
      if (number_of_states_in_block(index_block_B)<number_of_states_in_block(second_block_B))
      {
        m_constellations[ci].blocks.pop_front();
      }
      else
      { 
        m_constellations[ci].blocks.erase_after(m_constellations[ci].blocks.begin());
        index_block_B=second_block_B;
      }
      return index_block_B;
#else
DIT WERKT NIET MEER WANT NON_TRIVIAL_CONSTELLATIONS IS NU EEN VECTOR EN GEEN SET. Het maakte ook weinig verschil in performance. 
      // Find the smallest constellation to be split off. Do not use more than a constant search steps more than the size of the found block.
      // the algorithm. 
      std::size_t search_steps=0; 
//std::cerr << "FIND BEST BLOCK SIZE ----------------------------------------------------------------------------\n";
      constellation_index best_constellation=-1;
      block_index best_block=-1;
      std::size_t smallest_block_size=-1;
      std::forward_list<block_index>::const_iterator lagging_pointer_to_best_block;   // Equal to end, if the best block is at the beginning.
      bool stop=false;
      for(set_of_constellations_type::const_iterator c=m_non_trivial_constellations.begin();
                   !stop && c!=m_non_trivial_constellations.end(); c++)
      {
        const std::forward_list<block_index>& blocks=m_constellations[*c].blocks;
        std::forward_list<block_index>::const_iterator lagging_pointer=blocks.end();
        for(std::forward_list<block_index>::const_iterator bi=blocks.begin(); !stop && bi!=blocks.end(); bi++)
        {
          std::size_t size_bi=number_of_states_in_block(*bi);
//std::cerr << "BLOCK SIZE " << size_bi << "\n";
          if (size_bi<smallest_block_size && size_bi>=search_steps)  // We amortize the number of search steps on the block that is split off
          {
//std::cerr << "BEST FOUND BLOCK SIZE " << size_bi << "\n";
            smallest_block_size=size_bi;
            best_constellation= *c;
            best_block=*bi;
            lagging_pointer_to_best_block=lagging_pointer;
          }
          lagging_pointer=bi;
          search_steps++;
          if (smallest_block_size==1 || search_steps>smallest_block_size)
          {
            stop=true;
          }
        }
      }
      if (lagging_pointer_to_best_block==m_constellations[best_constellation].blocks.end())
      {
        m_constellations[best_constellation].blocks.pop_front();
      }
      else 
      {
        m_constellations[ci].blocks.erase_after(lagging_pointer_to_best_block);
      }
//std::cerr << "RESULTING BLOCK SIZE " << smallest_block_size << "\n";
      ci=best_constellation;
      return best_block;
#endif
    }


    void refine_partition_until_it_becomes_stable()
    {
      // This represents the while loop in Algorithm 1 from line 1.6 to 1.25.

      std::vector<label_count_sum_tuple> value_counter(m_blocks.size());
      std::vector<transition_index> count_transitions_per_label;
      count_transitions_per_label.reserve(m_aut.num_action_labels()); // will be reset to 0 later
      std::vector<label_index> todo_stack_labels;
      std::vector<block_index> todo_stack_blocks;
      std::vector<transition_index> calM;
      std::vector<transition_index> transitions_for_which_start_same_saC_must_be_repaired;
      // Algorithm 1, line 1.6.
      while (!m_non_trivial_constellations.empty())
      {
        /* static time_t last_log_time=time(nullptr)-1;
        time_t new_log_time = 0;
        if (time(&new_log_time)>last_log_time)
        {
          mCRL2log(log::verbose) << "Refining. There are " << m_blocks.size() << " blocks and " << m_constellations.size() << " constellations.\n";
          last_log_time=last_log_time = new_log_time;
        } */
        // print_data_structures("MAIN LOOP");
        assert(check_data_structures("MAIN LOOP"));
        assert(check_stability("MAIN LOOP"));

        // Algorithm 1, line 1.7.
        constellation_index ci=0;
        block_index index_block_B=select_and_remove_a_block_in_a_non_trivial_constellation(ci);
// mCRL2log(log::debug) << "REMOVE BLOCK " << index_block_B << " from constellation " << ci << "\n";

        // Algorithm 1, line 1.8.
        std::forward_list<block_index>::iterator fl=m_constellations[ci].blocks.begin();
        if (++fl == m_constellations[ci].blocks.end()) // Constellation is trivial.
        {
          assert(m_non_trivial_constellations.back()==ci);
          m_non_trivial_constellations.pop_back();
        }
        m_constellations.emplace_back(index_block_B);
        const constellation_index old_constellation=m_blocks[index_block_B].constellation;
        const constellation_index new_constellation=m_constellations.size()-1;
        m_blocks[index_block_B].constellation=new_constellation;
        #ifdef CHECK_COMPLEXITY_GJ
          // m_constellations[new_constellation].work_counter = m_constellations[old_constellation].work_counter;
          const unsigned max_C = check_complexity::log_n - check_complexity::ilog2(number_of_states_in_constellation(new_constellation));
          mCRL2complexity_gj(&m_blocks[index_block_B], add_work(check_complexity::refine_partition_until_it_becomes_stable__find_splitter, max_C), *this);
        #endif
        // Here the variables block_to_constellation and the doubly linked list L_B->C in blocks must be still be updated.
        // This happens further below.

        // Algorithm 1, line 1.9.
        block_label_to_size_t_map block_label_to_cotransition;
 
        // The following data structure maintains per state and action label from where to where the start_same_saC pointers
        // in m_outgoing_transitions still have to be set. 
 
        clear(todo_stack_labels);
// David suggests: The following statement takes time O(|Act|).
        count_transitions_per_label.assign(m_aut.num_action_labels(), 0);
        for(typename std::vector<state_index>::iterator i=m_blocks[index_block_B].start_bottom_states;
                                                        i!=m_blocks[index_block_B].end_states; ++i)
        {
          // mCRL2complexity_gj(m_states[*i], add_work(check_complexity::..., max_C), *this);
              // subsumed under the above counter
          // and visit the incoming transitions.
          /* const std::vector<transition_index>::iterator end_it=
                          ((*i)+1==m_states_in_blocks.size())?m_incoming_transitions.end()
                                                             :m_states[(*i)+1].start_incoming_inert_transitions;
          for(std::vector<transition_index>::iterator j=m_states[*i].start_incoming_inert_transitions; j!=end_it; ++j)
          {
            const transition& t=m_aut.get_transitions()[*j];
            if (!is_inert_during_init(t) || m_states[t.from()].block!=m_states[t.to()].block)  NOTE: mogelijk kan laatste check weg door alleen door
                                                                                               non-inert transitions te lopen. 
            {
              transition_index& c=count_transitions_per_label[t.label()];
              if (c==0)
              {
                todo_stack_labels.push_back(t.label());
              }    
              c++;
            }
          } */
          const std::vector<transition>::iterator end_it=
                          ((*i)+1==m_states.size())?m_aut.get_transitions().end()
                                                   :m_states[(*i)+1].start_incoming_transitions;
          for(std::vector<transition>::iterator j=m_states[*i].start_incoming_transitions; j!=end_it; ++j)
          {
            const transition& t=*j;
            // mCRL2complexity_gj(&m_transitions[std::distance(m_aut.get_transitions().begin(), j)], add_work(check_complexity::..., max_C), *this);
                // subsumed under the above counter
            if (// !is_inert_during_init(t) || m_states[t.from()].block!=m_states[t.to()].block
                // ignore tau-self-loops even if divergence is preserved, because we assume that blocks are stable under these already.
                !m_branching || !m_aut.is_tau(m_aut.apply_hidden_label_map(t.label())) || m_states[t.from()].block != m_states[t.to()].block)
            {
              transition_index& c=count_transitions_per_label[t.label()];
              if (c==0)
              {
                todo_stack_labels.push_back(t.label());
              }    
              c++;
            }
          }
        }
        transition_index size_of_states_per_action_label=accumulate_entries(count_transitions_per_label, todo_stack_labels);
        calM.resize(size_of_states_per_action_label);
        clear(transitions_for_which_start_same_saC_must_be_repaired);

        // Walk through all states in block B
        for(typename std::vector<state_index>::iterator i=m_blocks[index_block_B].start_bottom_states;
                                                        i!=m_blocks[index_block_B].end_states; ++i)
        {
          // mCRL2complexity_gj(m_states[*i], add_work(check_complexity::..., max_C), *this);
              // subsumed under the above counter
          // and visit the incoming transitions. 
          /* const std::vector<transition_index>::iterator end_it=
                          ((*i)+1==m_states_in_blocks.size())?m_incoming_transitions.end()
                                                             :m_states[(*i)+1].start_incoming_inert_transitions;
          for(std::vector<transition_index>::iterator j=m_states[*i].start_incoming_inert_transitions; j!=end_it; ++j)
          {
            const transition& t=m_aut.get_transitions()[*j];
            
            // Add the source state grouped per label in calM, provided the transition is non inert.
            if (!is_inert_during_init(t) || m_states[t.from()].block!=m_states[t.to()].block)
            {
              transition_index& c=count_transitions_per_label[t.label()];
              calM[c]=*j;
              c++;
            } */
 
          const std::vector<transition>::iterator end_it=
                          ((*i)+1==m_states.size())?m_aut.get_transitions().end()
                                                   :m_states[(*i)+1].start_incoming_transitions;
          for(std::vector<transition>::iterator j=m_states[*i].start_incoming_transitions; j!=end_it; ++j)
          {
            const transition& t=*j;
            const transition_index t_index=std::distance(m_aut.get_transitions().begin(),j);
            // mCRL2complexity_gj(&m_transitions[t_index], add_work(check_complexity::..., max_C), *this);
                // subsumed under the above counter
            
            // Add the source state grouped per label in calM, provided the transition is non inert.
            // (need to ignore tau-self-loops the same way as above)
            if (!m_branching || !m_aut.is_tau(m_aut.apply_hidden_label_map(t.label())) || m_states[t.from()].block != m_states[t.to()].block)
            {
              transition_index& c=count_transitions_per_label[t.label()];
              calM[c]=t_index;
              c++;
            }
 
// David suggests: Move the following code to update the saC references to the first
// loop through the states in block B.
// Here, place the code from below to correct the start_same_saC pointers.
// Then we do not need the vector transitions_for_which_start_same_saC_must_be_repaired.
            // Update the state-action-constellation (saC) references in m_outgoing_transitions.
            const outgoing_transitions_it pos1=m_transitions[t_index].ref_outgoing_transitions;
            outgoing_transitions_it end_same_saC;
            if (pos1->start_same_saC<pos1)
            {
              // This is the last transition with the same saC. 
              end_same_saC=pos1;
            }
            else
            {  
              end_same_saC=pos1->start_same_saC;
            }
            const outgoing_transitions_it pos2=end_same_saC->start_same_saC;
            if (pos1!=pos2)
            {
              std::swap(pos1->transition,pos2->transition);
              m_transitions[m_BLC_transitions[pos1->transition]].ref_outgoing_transitions=pos1;
              m_transitions[m_BLC_transitions[pos2->transition]].ref_outgoing_transitions=pos2;
            }
            if (end_same_saC->start_same_saC<end_same_saC)
            {
              end_same_saC->start_same_saC++;
            }
       
            transitions_for_which_start_same_saC_must_be_repaired.push_back(t_index);
            
            // Update the block_label_to_cotransition map.
            if (block_label_to_cotransition.find(std::pair(m_states[t.from()].block,m_aut.apply_hidden_label_map(t.label())))==block_label_to_cotransition.end())
            {
              // Not found. Add a transition from the LBC_list to block_label_to_cotransition
              // that goes to C\B, or the null_transition if no such transition exists, which prevents searching
              // the list again. Except if t.from is in C\B and a=tau, because in that case it is a (former) constellation-inert transition.
              bool found=false;

              if (!m_branching || !m_aut.is_tau(m_aut.apply_hidden_label_map(t.label())) || m_blocks[m_states[t.from()].block].constellation!=ci)
              {
                LBC_list_iterator transition_walker=m_transitions[t_index].transitions_per_block_to_constellation->start_same_BLC;
                
                while (!found && transition_walker!=m_transitions[t_index].transitions_per_block_to_constellation->end_same_BLC)
                {
// THIS TAKES TOO LONG. THERE MAY BE MANY TRANSITIONS FROM THE SOURCE BLOCK OF t
// TO THE LARGE CONSTELLATION ci THAT NEED TO BE SEARCHED.
// INSTEAD, PLEASE USE OTHER MEANS, LIKE THE BLC JUST BEFORE OR AFTER IN MEMORY.
                  // mCRL2complexity_gj(&m_transitions[*transition_walker], add_work(check_complexity::refine_partition_until_it_becomes_stable__find_cotransition, max_C), *this);
                  const transition& tw=m_aut.get_transitions()[*transition_walker];
                  assert(m_states[tw.from()].block == m_states[t.from()].block);
                  assert(m_aut.apply_hidden_label_map(tw.label()) == m_aut.apply_hidden_label_map(t.label()));
                  if (m_blocks[m_states[tw.to()].block].constellation==ci)
                  {
                    found=true;
                    block_label_to_cotransition[std::pair(m_states[t.from()].block, m_aut.apply_hidden_label_map(t.label()))]= *transition_walker;
                  }
                  else
                  {
                    ++transition_walker;
                  }
                }
              }
              if (!found)
              {
                block_label_to_cotransition[std::pair(m_states[t.from()].block,m_aut.apply_hidden_label_map(t.label()))]= null_transition;
              }
            }
            // Update the doubly linked list L_B->C in blocks as the constellation is split in B and C\B. 
            update_the_doubly_linked_list_LBC_new_constellation(index_block_B, t, t_index);
          }
        }
        
        while (!transitions_for_which_start_same_saC_must_be_repaired.empty())
        {
          transition_index ti=transitions_for_which_start_same_saC_must_be_repaired.back();
          transitions_for_which_start_same_saC_must_be_repaired.pop_back();
          // mCRL2complexity_gj(&m_transitions[ti], add_work(..., max_C), *this);
              // this can be subsumed under the above call
              // because in every iteration we add at most one element to transitions_for_which_start_same_saC_must_be_repaired
          const outgoing_transitions_it outgoing_it=m_transitions[ti].ref_outgoing_transitions;
          const transition& t=m_aut.get_transitions()[ti];
          if ((outgoing_it+1)!=m_outgoing_transitions.end())
          {
            const transition& t_next=m_aut.get_transitions()[m_BLC_transitions[(outgoing_it+1)->transition]];
            if (t.from()==t_next.from() && m_aut.apply_hidden_label_map(t.label())==m_aut.apply_hidden_label_map(t_next.label()) &&
                m_blocks[m_states[t.to()].block].constellation==m_blocks[m_states[t_next.to()].block].constellation &&
                (!m_preserve_divergence || !m_aut.is_tau(m_aut.apply_hidden_label_map(t.label())) || (t_next.from() != t_next.to() && (assert(t.from() != t.to()), true))))
            {
              outgoing_it->start_same_saC=(outgoing_it+1)->start_same_saC;
              outgoing_it->start_same_saC->start_same_saC=outgoing_it; // let the last transition point to this transitions,
                                                                       // as this one is first, due to the use of the repair vector as a stack.. 
            }
            else 
            {
              outgoing_it->start_same_saC=outgoing_it;
            }
          }
          else 
          {
            outgoing_it->start_same_saC=outgoing_it;
          }
        }

        // ---------------------------------------------------------------------------------------------
        // First carry out a co-split of B with respect to C\B and an action tau.
        /* bool hatU_does_not_cover_B_bottom=false;
        for(typename std::vector<state_index>::iterator si=m_blocks[index_block_B].start_bottom_states;
                          !hatU_does_not_cover_B_bottom &&
                          si!=m_blocks[index_block_B].start_non_bottom_states; 
                        ++si)
        {
          bool found=false;
          const outgoing_transitions_it end_it=((*si)+1>=m_states.size())?m_outgoing_transitions.end():m_states[(*si)+1].start_outgoing_transitions;
          for(outgoing_transitions_it tti=m_states[*si].start_outgoing_transitions;
                                       !found && tti!=end_it;
                                       ++tti)
          { 
            const transition& t=m_aut.get_transitions()[m_BLC_transitions[tti->transition]];
            if (is_inert_during_init(t) && m_blocks[m_states[t.to()].block].constellation==old_constellation)
            { 
              found =true;
            }
          }
          if (!found)
          {
            // This state has notransition to the old constellation. 
            hatU_does_not_cover_B_bottom=true;
          }
        } */
        if (m_branching)
        {
          transition_index co_t=find_inert_co_transition_for_block(index_block_B, old_constellation);

          // Algorithm 1, line 1.19.
          if (co_t!=null_transition &&
              // The routine below has a side effect, as it sets m_R and m_U for all bottom states of block B.
              hatU_does_not_cover_B_bottom(index_block_B, old_constellation))
          {
          // Algorithm 1, line 1.10.
            
//std::cerr << "DO A TAU CO SPLIT " << old_constellation << "\n";
            bool dummy=false;
            splitB<2>(index_block_B, 
                        m_transitions[co_t].transitions_per_block_to_constellation->start_same_BLC, 
                        m_transitions[co_t].transitions_per_block_to_constellation->end_same_BLC, 
                        m_blocks[index_block_B].start_bottom_states, 
                        m_blocks[index_block_B].start_non_bottom_states,
                        m_aut.tau_label_index(),
                        old_constellation,
                        dummy,
                        [&block_label_to_cotransition, ci, this]
                          (const block_index old_block, 
                           const block_index new_block,
                           const transition_index moved_transition,
                           const transition_index alternative_transition)
                          {
                            maintain_block_label_to_cotransition(
                                    old_block,
                                    new_block,
                                    moved_transition,
                                    alternative_transition, block_label_to_cotransition,
                                    ci);
                          });
          }
        }
        // Algorithm 1, line 1.10.
        std::vector<transition_index>::iterator start_index=calM.begin();
        clear(todo_stack_blocks);
        for(const label_index a: todo_stack_labels)
        {
          // mCRL2complexity_gj(..., add_work(..., max_C), *this);
              // not needed as the inner loop is always executed at least once.
//std::cerr << "ACTION " << m_aut.action_label(a) << " target block " << index_block_B << "\n";
          // print_data_structures("Main loop");
          assert(check_data_structures("Main loop", false, false));
          // Algorithm 1, line 1.11.
          value_counter.resize(m_blocks.size());
          assert(todo_stack_blocks.empty());
          std::vector<transition_index>::iterator end_index=calM.begin()+count_transitions_per_label[a];
          group_in_situ(start_index,
                        end_index,
                        todo_stack_blocks,
                        value_counter);

          assert(!todo_stack_blocks.empty()); // ensure that the inner loop is executed
          std::vector<transition_index>::iterator start_index_per_block=start_index;
          for(const block_index bi: todo_stack_blocks)
          {
//std::cerr << "INVESTIGATE ACTION " << m_aut.action_label(a) << " source block " << bi << " target block " << index_block_B << "\n";
            std::vector<transition_index>::iterator end_index_per_block=start_index+value_counter[bi].not_investigated;
            assert(start_index_per_block!=end_index_per_block);
            mCRL2complexity_gj(m_transitions[*start_index_per_block].transitions_per_block_to_constellation, add_work(check_complexity::refine_partition_until_it_becomes_stable__select_action_label_and_block_to_be_split, max_C), *this);
            block_index Bpp=bi;
            // Check whether the bottom states of bi are not all included in Mleft. 
            // This function has a side effect if not all states are covered on m_P, m_counter_reset_vector and the counters in states. 
            const bool not_all_states = not_all_bottom_states_are_touched
                    (bi, start_index_per_block, end_index_per_block);
            if (not_all_states && m_R.empty())
            {
              // the BLC slice only contains inert transitions.
              // We do not need to do a main split, neither a co-split.
            }
            else
            {
              if (not_all_states)
              {
// mCRL2log(log::debug) << "PERFORM A MAIN SPLIT \n";
              // Algorithm 1, line 1.12.
                bool M_in_bi1=true;
                // std::size_t dummy_number=0;
                block_index bi1=splitB<1>(bi,
                                        start_index_per_block,
                                        end_index_per_block,
                                        m_blocks[bi].start_bottom_states, 
                                        m_blocks[bi].start_non_bottom_states, 
                                        a, // action index (required for bookkeeping)
                                        new_constellation, // constellation index (required for bookkeeping)
                                        M_in_bi1,
                                        [&block_label_to_cotransition, ci, this]
                                          (const block_index old_block, 
                                           const block_index new_block,
                                           const transition_index moved_transition,
                                           const transition_index alternative_transition)
                                          {
                                            maintain_block_label_to_cotransition(
                                                    old_block,
                                                    new_block,
                                                    moved_transition,
                                                    alternative_transition, 
                                                    block_label_to_cotransition,
                                                    ci);
                                          });
                assert(0 <= bi1 && bi1 < m_blocks.size());
                // Algorithm 1, line 1.13.
                if (M_in_bi1)
                {
                  Bpp=bi1;
                }
                // Algorithm 1, line 1.14 is implicitly done in the call of splitB above.
              }
              // Algorithm 1, line 1.17 and line 1.18.
// mCRL2log(log::debug) << "BLOCK THAT IS SPLITTER " << Bpp << "\n";
              typename block_label_to_size_t_map::const_iterator bltc_it=block_label_to_cotransition.find(std::pair(Bpp,a));

              if (bltc_it!=block_label_to_cotransition.end() &&
                bltc_it->second!=null_transition &&
                some_bottom_state_has_no_outgoing_co_transition(Bpp, start_index_per_block, end_index_per_block, old_constellation))
              {
// mCRL2log(log::debug) << "CO-TRANSITION  " << ptr(bltc_it->second) << "\n";
                // Algorithm 1, line 1.19.
              
                bool dummy=false;
// mCRL2log(log::debug) << "PERFORM A MAIN CO-SPLIT \n";
                splitB<2>(Bpp,
                        m_transitions[bltc_it->second].transitions_per_block_to_constellation->start_same_BLC,
                        m_transitions[bltc_it->second].transitions_per_block_to_constellation->end_same_BLC, 
                        m_blocks[Bpp].start_bottom_states, 
                        m_blocks[Bpp].start_non_bottom_states,
                        a,
                        old_constellation,
                        dummy,
                        [&block_label_to_cotransition, ci, this]
                          (const block_index old_block, 
                           const block_index new_block,
                           const transition_index moved_transition,
                           const transition_index alternative_transition)
                          {
                            maintain_block_label_to_cotransition(
                                    old_block,
                                    new_block,
                                    moved_transition,
                                    alternative_transition, block_label_to_cotransition,
                                    ci);
                          });
                // Algorithm 1, line 1.20 and 1.21. P is updated implicitly when splitting Bpp.

              }
            }
            start_index_per_block=end_index_per_block;
          }
          assert(start_index_per_block == end_index);
          start_index=end_index;
          todo_stack_blocks.clear();
        }
        
        assert(check_data_structures("Before stabilize", false, false));
        stabilizeB();
      }
    }
};

/* ************************************************************************* */
/*                                                                           */
/*                             I N T E R F A C E                             */
/*                                                                           */
/* ************************************************************************* */


/// \brief nonmember functions serving as interface with the rest of mCRL2
/// \details These functions are copied, almost without changes, from
/// liblts_bisim_gw.h, which was written by Anton Wijs.

/// \brief Reduce transition system l with respect to strong or
/// (divergence-preserving) branching bisimulation.
/// \param[in,out] l                   The transition system that is reduced.
/// \param         branching           If true branching bisimulation is
///                                    applied, otherwise strong bisimulation.
/// \param         preserve_divergence Indicates whether loops of internal
///                                    actions on states must be preserved.  If
///                                    false these are removed.  If true these
///                                    are preserved.
template <class LTS_TYPE>
void bisimulation_reduce_gj(LTS_TYPE& l, const bool branching = false,
                                         const bool preserve_divergence = false)
{
    if (1 >= l.num_states())
    {
        mCRL2log(log::warning) << "There is only 1 state in the LTS. It is not "
                "guaranteed that branching bisimulation minimisation runs in "
                "time O(m log n).\n";
    }
    // Line 1.2: Find tau-SCCs and contract each of them to a single state
mCRL2log(log::verbose) << "Start SCC\n";
    if (branching)
    {
        scc_reduce(l, preserve_divergence);
    }

    // Now apply the branching bisimulation reduction algorithm.  If there
    // are no taus, this will automatically yield strong bisimulation.
mCRL2log(log::verbose) << "Start Partitioning\n";
    bisim_partitioner_gj<LTS_TYPE> bisim_part(l, branching, preserve_divergence);

    // Assign the reduced LTS
mCRL2log(log::verbose) << "Start finalizing\n";
    bisim_part.finalize_minimized_LTS();
}


/// \brief Checks whether the two initial states of two LTSs are strong or
/// (divergence-preserving) branching bisimilar.
/// \details This routine uses the O(m log n) branching bisimulation algorithm
/// developed in 2018 by David N. Jansen.  It runs in O(m log n) time and uses
/// O(m) memory, where n is the number of states and m is the number of
/// transitions.
///
/// The LTSs l1 and l2 are not usable anymore after this call.
/// \param[in,out] l1                  A first transition system.
/// \param[in,out] l2                  A second transistion system.
/// \param         branching           If true branching bisimulation is used,
///                                    otherwise strong bisimulation is
///                                    applied.
/// \param         preserve_divergence If true and branching is true, preserve
///                                    tau loops on states.
/// \param         generate_counter_examples  (non-functional, only in the
///                                    interface for historical reasons)
/// \returns True iff the initial states of the transition systems l1 and l2
/// are ((divergence-preserving) branching) bisimilar.
template <class LTS_TYPE>
bool destructive_bisimulation_compare_gj(LTS_TYPE& l1, LTS_TYPE& l2,
        const bool branching = false, const bool preserve_divergence = false,
        const bool generate_counter_examples = false,
        const std::string& /*counter_example_file*/ = "", bool /*structured_output*/ = false)
{
    if (generate_counter_examples)
    {
        mCRL2log(log::warning) << "The GJ24 branching bisimulation "
                              "algorithm does not generate counterexamples.\n";
    }
    std::size_t init_l2(l2.initial_state() + l1.num_states());
    detail::merge(l1, std::move(l2));
    l2.clear(); // No use for l2 anymore.

    // Line 2.1: Find tau-SCCs and contract each of them to a single state
    if (branching)
    {
        detail::scc_partitioner<LTS_TYPE> scc_part(l1);
        scc_part.replace_transition_system(preserve_divergence);
        init_l2 = scc_part.get_eq_class(init_l2);
    }                                                                           else  assert(!preserve_divergence);
                                                                                assert(1 < l1.num_states());
    bisim_partitioner_gj<LTS_TYPE> bisim_part(l1, branching, preserve_divergence);

    return bisim_part.in_same_class(l1.initial_state(), init_l2);
}


/// \brief Checks whether the two initial states of two LTSs are strong or
/// (divergence-preserving) branching bisimilar.
/// \details The LTSs l1 and l2 are first duplicated and subsequently reduced
/// modulo bisimulation.  If memory is a concern, one could consider to use
/// destructive_bisimulation_compare().  This routine uses the O(m log n)
/// branching bisimulation algorithm developed in 2018 by David N. Jansen.  It
/// runs in O(m log n) time and uses O(m) memory, where n is the number of
/// states and m is the number of transitions.
/// \param l1                  A first transition system.
/// \param l2                  A second transistion system.
/// \param branching           If true branching bisimulation is used,
///                            otherwise strong bisimulation is applied.
/// \param preserve_divergence If true and branching is true, preserve tau
///                            loops on states.
/// \retval True iff the initial states of the transition systems l1 and l2
/// are ((divergence-preserving) branching) bisimilar.
template <class LTS_TYPE>
inline bool bisimulation_compare_gj(const LTS_TYPE& l1, const LTS_TYPE& l2,
          const bool branching = false, const bool preserve_divergence = false)
{
    LTS_TYPE l1_copy(l1);
    LTS_TYPE l2_copy(l2);
    return destructive_bisimulation_compare_gj(l1_copy, l2_copy, branching,
                                                          preserve_divergence);
}


} // end namespace detail
} // end namespace lts
} // end namespace mcrl2

#endif // ifndef LIBLTS_BISIM_GJ_H
