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
///

#ifndef LIBLTS_BISIM_GJ_H
#define LIBLTS_BISIM_GJ_H

#include <forward_list>
#include <deque>
#include "mcrl2/lts/detail/liblts_scc.h"
#include "mcrl2/lts/detail/liblts_merge.h"
// #include "mcrl2/lts/detail/check_complexity.h"
// #include "mcrl2/lts/detail/fixed_vector.h"


namespace mcrl2
{
namespace lts
{
namespace detail
{
namespace bisimulation_gj
{

// Forward declaration.
struct block_type;
struct transition_type;

typedef std::size_t state_index;
typedef std::size_t action_index;
typedef std::size_t transition_index;
typedef std::size_t block_index;
typedef std::size_t constellation_index;

constexpr transition_index null_transition=-1;

// Below the four main data structures are listed.
struct state_type_gj
{
  block_index block=0;
  std::vector<transition_index>::iterator start_incoming_transitions;
  std::vector<transition_index>::iterator start_outgoing_inert_transitions;
  std::vector<transition_index>::iterator start_outgoing_non_inert_transitions;
  std::vector<state_index>::iterator ref_states_in_blocks;
};

struct transition_type
{
  // The position of the transition type corresponds to m_aut.get_transitions(). 
  // std::size_t from, label, to are found in m_aut.transitions.
  std::forward_list<transition_index > :: const_iterator transitions_per_block_to_constellation;
  transition_index next_L_B_C_element;
  transition_index previous_L_B_C_element;
  std::vector<std::size_t>::iterator trans_count;
};

struct block_type
{
  constellation_index constellation;
  std::vector<state_index>::iterator start_bottom_states;
  std::vector<state_index>::iterator start_non_bottom_states;
  std::forward_list< transition_index > block_to_constellation;

  block_type(const std::vector<state_index>::iterator beginning_of_states, constellation_index c)
    : constellation(c),
      start_bottom_states(beginning_of_states),
      start_non_bottom_states(beginning_of_states)
  {}
};

struct constellation_type
{
  block_index start_block;
  block_index end_block;

  constellation_type(const block_index start, const block_index end)
   : start_block(start), 
     end_block(end)
  {}
};

// The struct below facilitates to walk through a L_B_C_list starting from an arbitrary transition.
struct L_B_C_list_iterator
{
  bool walk_backwards=true;
  const transition_index m_start_transition;
  transition_index m_current_transition;
  const std::vector<transition_type>& m_transitions;

  L_B_C_list_iterator(const transition_index ti, const std::vector<transition_type>& transitions)
    : m_start_transition(ti),
      m_current_transition(ti),
      m_transitions(transitions)
  {}

  void operator++()
  {
    if (walk_backwards)
    {
      m_current_transition=m_transitions[m_current_transition].previous_L_B_C_element;
      if (m_current_transition==null_transition)
      {
        walk_backwards=false;
        m_current_transition=m_transitions[m_start_transition].next_L_B_C_element;
      }
    }
    else
    {
      m_current_transition=m_transitions[m_current_transition].previous_L_B_C_element;
    }
  }

  transition_index operator *() const
  {
    return m_current_transition;
  }

  // Equality is implemented minimally for the purpose of this algorithm,
  // essentially only intended to compare the iterator to its end, i.e., null_transition. 
  bool operator ==(const L_B_C_list_iterator& it) const
  {
    return m_current_transition==it.m_current_transition;
  }
};

/* struct L_B_C_list_walker
{
  typedef L_B_C_list_iterator iterator;

  const transition_index start_transition;
  std::vector<transition_type>& m_transitions;
  

  L_B_C_list_walker(const transition_index ti, const std::vector<transition_type>& transitions)
    : start_transtition(ti),
      m_transitions.
  {}
} */

} // end namespace bisimulation_gj


/*=============================================================================
=                                 main class                                  =
=============================================================================*/


using namespace mcrl2::lts::detail::bisimulation_gj;



/// \class bisim_partitioner_gj
/// \brief implements the main algorithm for the branching bisimulation
/// quotient
template <class LTS_TYPE>
class bisim_partitioner_gj
{
  protected:
    typedef typename LTS_TYPE::labels_size_type label_index;
    typedef typename LTS_TYPE::states_size_type state_index;
    typedef std::unordered_set<state_index> set_of_states_type;
    typedef std::unordered_set<constellation_index> set_of_constellations_type;
    typedef std::unordered_map<label_index, set_of_states_type > states_per_action_label_type;
    typedef std::unordered_map<block_index, set_of_states_type > states_per_block_type;
    typedef std::unordered_map<std::pair<state_index, label_index>, std::size_t> state_label_to_size_t_map;
    typedef std::unordered_map<label_index, std::size_t> label_to_size_t_map;

    set_of_states_type& empty_state_set()
    {
      static const set_of_states_type s=set_of_states_type();
      return s;
    }

    /// \brief automaton that is being reduced
    LTS_TYPE& m_aut;
    
    // Generic data structures.
    std::vector<state_type_gj> m_states;
    std::vector<std::reference_wrapper<transition_type>> m_incoming_transitions;
    std::vector<std::reference_wrapper<transition_type>> m_outgoing_transitions;
    std::vector<transition_type> m_transitions;
    std::deque<std::size_t> m_state_to_constellation_count;
    std::vector<state_index> m_states_in_blocks;
    std::vector<block_type> m_blocks;
    std::vector<constellation_type> m_constellations;
    set_of_states_type m_P;

    /// \brief true iff branching (not strong) bisimulation has been requested
    const bool m_branching;
  
    /// \brief true iff divergence-preserving branching bisimulation has been
    /// requested
    /// \details Note that this field must be false if strong bisimulation has
    /// been requested.  There is no such thing as divergence-preserving strong
    /// bisimulation.
    const bool m_preserve_divergence;

    /// The following variable contains all non trivial constellations.
    set_of_constellations_type m_non_trivial_constellations;

    std::size_t number_of_states_in_block(const block_index B) const
    {
      if (m_blocks.size()==B+1) // This is the last block.
      {
        return m_states_in_blocks.size()-m_blocks[B].start_bottom_states; 
      }
      return m_blocks[B+1].start_bottom_states-m_blocks[B].start_bottom_states; 
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
        m_constellations(1,constellation_type(0,1)),   // Algorithm 1, line 1.2.
        m_branching(branching),
        m_preserve_divergence(preserve_divergence)
    {                                                                           
      assert(m_branching || !m_preserve_divergence);
      create_initial_partition();        
      refine_partition_until_it_becomes_stable();
    }


    /// \brief Calculate the number of equivalence classes
    /// \details The number of equivalence classes (which is valid after the
    /// partition has been constructed) is equal to the number of states in the
    /// bisimulation quotient.
    state_type_gj num_eq_classes() const
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
    state_type_gj get_eq_class(const state_type_gj s) const
    {
      assert(s<m_blocks.size());
      return m_states[s].block;
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
      std::unordered_set<transition> T;
      for(const transition& t: m_aut.get_transitions())
      {
        T.insert(transition(get_eq_class(t.from()), t.label(), t.to()));
      }
      for (const transition t: T)
      {
        m_aut.add_transition(t);
      }

      // Merge the states, by setting the state labels of each state to the
      // concatenation of the state labels of its equivalence class.

      if (m_aut.has_state_info())   /* If there are no state labels this step is not needed */
      {
        /* Create a vector for the new labels */
        std::vector<typename LTS_TYPE::state_label_t> new_labels(num_eq_classes());

    
        for(std::size_t i=0; i<m_aut.num_states(); ++i)
        {
          const state_type_gj new_index(get_eq_class(i));
          new_labels[new_index]=new_labels[new_index]+m_aut.state_label(i);
        }

        m_aut.set_num_states(num_eq_classes());
        for (std::size_t i=0; i<num_eq_classes(); ++i)
        {
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

    /*--------------------------- main algorithm ----------------------------*/

    
    // Traverse the states in block co_bi to determine which of the states in 
    // block bi did become new bottom states. 
    void check_transitions_that_became_non_inert_in_a_block_by_co_block(
                          const block_index /* bi */, 
                          const block_index /* co_bi */)
    {
      assert(0);
      // XXX TODO.
    }

    // Traverse the states in block bi and determine which did become bottom states.
    void check_transitions_that_became_non_inert_in_a_block(
                          const block_index bi) 
                          
    {
      for(typename std::vector<state_index>::iterator si=m_blocks[bi].start_non_bottom_states; 
                 si!=m_states_in_blocks.end() &&
                 (bi+1==m_blocks.size() || si!=m_blocks[bi+1].start_bottom_states); 
                 ++si)
      {
        const state_type_gj& s= m_states[si];
        bool no_inert_transition_seen=true;
        for(std::vector<transition_index>::iterator ti=s.start_incoming_transitions; 
                    m_aut.is_tau(m_aut.get_transitions()[ti].label()) && 
                    ti!=m_incoming_transitions.end() &&
                    ti!=(si+1)->get().start_incoming_transitions;
                ti++)
        {
          if (m_states[m_aut.get_transitions()[ti].from()].block==m_states[m_aut.get_transitions()[ti].to()].block)
          {
            // This transition is inert.
            no_inert_transition_seen=false;
            ti++;
          }
          else
          {
            // This transition is non-inert
          }
        }
        if (no_inert_transition_seen)
        {
          m_P[bi].insert(*si);
        }
      }
    }

    // Move transitions that became non-inert to their proper place in 
    //     m_incoming_transitions
    //     m_outgoing_transitions
    //     m_states_in_blocks
    // Moreover, record the states that became bottom states in m_P;
    void check_transitions_that_became_non_inert()
    {
      // Calculate the newly obtained bottom states and put them in m_P. 
      // We walk through all non-bottom states of blocks.
      for(block_index bi=0; bi<m_blocks.size(); ++bi)
      {
        check_transitions_that_became_non_inert_in_a_block(bi);
      }
    }

    /*----------------- SplitB -- Algorithm 3 of [GJ 2024] -----------------*/

    void swap_states_in_states_in_block(block_index pos1, block_index pos2, block_index pos3)
    {
      block_index temp=m_states_in_blocks[pos3];
      m_states_in_blocks[pos3]=m_states_in_blocks[pos2];
      m_blocks[m_states_in_blocks[pos3]].ref_states_in_block=pos3;
      m_states_in_blocks[pos2]=m_states_in_blocks[pos1];
      m_blocks[m_states_in_blocks[pos2]].ref_states_in_block=pos2;
      m_states_in_blocks[pos1]=temp;
      m_blocks[m_states_in_blocks[pos1]].ref_states_in_block=pos1;
    }

    block_index split_block_B_into_R_and_BminR(const block_index B, const std::unordered_set<state_index>& R)
    {
      m_blocks.emplace_back(m_blocks[B].bottom_states,m_blocks[B].constellation);
      m_non_trivial_constellations.insert(m_blocks[B].constellation);
      const block_index new_block_index=m_blocks.size()-1;
      for(state_index s: R)
      {
        m_states[s].block=new_block_index;
        std::size_t pos=m_states[s].ref_states_in_block;
        if (pos>=m_blocks[B].non_bottom_states) // the state is a non bottom state.
        {
          swap_states_in_states_in_block(pos,m_blocks[B].bottom_states,m_blocks[B].non_bottom_states);
        }
        else // the state is a non bottom state
        {
          swap_states_in_states_in_block(pos,m_blocks[new_block_index].bottom_states,m_blocks[B].non_bottom_states);
        }
        m_blocks[B].bottom_states++;
        m_blocks[B].non_bottom_states++;
      }
      return new_block_index;
    }
    
    void insert_in_the_doubly_linked_list_L_B_C_in_blocks(
               const transition& t,
               const transition_index ti,
               std::forward_list<transition_index > :: iterator position)
    {
      std::forward_list<transition_index > :: iterator this_block_to_constellation=
                                      m_transitions[ti].transitions_per_block_to_constellation;
      transition_index& current_transition_index= *position;
      // Check whether this is an inert transition.
      if (m_aut.is_tau(t.label()) &&
          m_states[t.from()].block==m_states[t.to()].block)
      {
        // insert before current transition.
        m_transitions[ti].next_L_B_C_element=current_transition_index;
        m_transitions[ti].previous_L_B_C_element=m_transitions[ti].previous_L_B_C_element;;
        if (m_transitions[current_transition_index].previous_L_B_C_element!=null_transition)
        {
          m_transitions[m_transitions[current_transition_index].previous_L_B_C_element].next_L_B_C_element=ti;
        }
        m_transitions[current_transition_index].previous_L_B_C_element=ti;
      }
      else
      {
        // insert after current transition.
        m_transitions[ti].previous_L_B_C_element=current_transition_index;
        m_transitions[ti].next_L_B_C_element=m_transitions[ti].previous_L_B_C_element;;
        if (m_transitions[current_transition_index].next_L_B_C_element!=null_transition)
        {
          m_transitions[m_transitions[current_transition_index].next_L_B_C_element].previous_L_B_C_element=ti;
        }
        m_transitions[current_transition_index].next_L_B_C_element=ti;
      }
    }

    // Move the transition t with transition index ti to an new 
    // L_B_C list if its source state is in block B and the target state switches to a new constellation.
    
    void update_the_doubly_linked_list_L_B_C_same_block(
               const block_index index_block_B, 
               const transition& t,
               const transition_index ti)
    {
      if (m_states[t.from()].block==index_block_B)
      {
        std::forward_list<transition_index > :: iterator this_block_to_constellation=
                             m_transitions[ti].transitions_per_block_to_constellation;
        std::forward_list<transition_index > :: iterator next_block_to_constellation=
                             ++std::forward_list<transition_index > :: iterator(this_block_to_constellation);
        if (next_block_to_constellation==m_blocks[m_states[t.from()].block].end() ||
            *next_block_to_constellation==null_transition ||
            m_blocks[m_aut.get_transitions()[*next_block_to_constellation].to()]!=index_block_B ||
            m_aut.get_transitions()[*next_block_to_constellation].label()!=t.label())
        { 
          // Make a new entry in the list next_block_to_constellation;
          next_block_to_constellation=
                  m_blocks[m_states[m_transitions[ti].from()].block].block_to_constellation.
                           insert_after(this_block_to_constellation, ti);
        }
        // Move the current transition to the next list.
        // First check whether this_block_to_constellation contains exactly transition ti.
        // It must be replaced by a later or earlier element from the L_B_C_list.
        bool last_element_removed=remove_from_the_doubly_linked_list_L_B_C_in_blocks(ti);
        insert_in_the_doubly_linked_list_L_B_C_in_blocks(t, ti, next_block_to_constellation);
        
        if (last_element_removed)
        {
          // move the L_B_C_list in next_block_to_constellation to block_to_constellation
          // and remove the next element.
          *this_block_to_constellation = *next_block_to_constellation;
          m_blocks[m_states[m_transitions[ti].from()].block].this_block_to_constellation.
                        erase_after(this_block_to_constellation);
        }
      }
    }

    // Update the L_B_C list of a transition, when the from state of the transition moves
    // from block old_bi to new_bi. 
    void update_the_doubly_linked_list_L_B_C_new_block(
               const block_index old_bi,
               const block_index new_bi,
               const transition& t,
               const transition_index ti,
               std::unordered_set< std::pair <action_index, constellation_index>, 
                            std::forward_list< transition_index >::iterator>& new_LBC_list_entries)
    {
      assert(m_states[t.from()].block==old_bi);
      
      std::forward_list<transition_index > :: iterator this_block_to_constellation=
                           m_transitions[ti].transitions_per_block_to_constellation;
      std::forward_list<transition_index > :: iterator next_block_to_constellation=
                           ++std::forward_list<transition_index > :: iterator(this_block_to_constellation);
      std::unordered_set< std::pair <action_index, constellation_index>,
                          std::forward_list< transition_index >::iterator>::iterator it=
                     new_LBC_list_entries.find(std::pair(t.label(), m_blocks[m_states[t.from()].block].constellation));
      bool last_element_removed=remove_from_the_doubly_linked_list_L_B_C_in_blocks(ti);
      if (it==new_LBC_list_entries.end())
      { 
        // Make a new entry in the list next_block_to_constellation;
        m_blocks[new_bi].next_block_to_constellation.push_front(ti);
        new_LBC_list_entries[std::pair(t.label(), m_blocks[m_states[t.from()].block].constellation)]=
                 m_blocks[new_bi].block_to_constellations.begin();
        transition_type tt=m_transitions[ti];
        tt.next_L_B_C_element=null_transition;
        tt.previous_L_B_C_element=null_transition;
      }
      else
      {
        // Move the current transition to the next list indicated by the iterator it.
        insert_in_the_doubly_linked_list_L_B_C_in_blocks(t, ti, it);
      }
      
      if (last_element_removed)
      {
        // move the L_B_C_list in next_block_to_constellation to block_to_constellation
        // and remove the next element.
        *this_block_to_constellation = *next_block_to_constellation;
        m_blocks[m_states[m_transitions[ti].from()].block].this_block_to_constellation.
                      erase_after(this_block_to_constellation);
      }
    }

    // Calculate the states R in block B that can inertly reach M and split
    // B in R and B\R. The complexity is conform the smallest block of R and B\R.
    // The L_B_C_list, trans_count and bottom states are not updated. 
    // Provide the index of the newly created block as a result. This block is the smallest of R and B\R.
    // Return in M_in_bi whether the new block bi is the one containing M.
    template <class MARKED_STATE_ITERATOR, 
              class UNMARKED_STATE_ITERATOR>
    block_index simple_splitB(const block_index B, 
                              const MARKED_STATE_ITERATOR M_begin, 
                              const MARKED_STATE_ITERATOR M_end, 
                              const std::function<bool(state_index)>& marked_blocker,
                              const UNMARKED_STATE_ITERATOR M_co_begin,
                              const UNMARKED_STATE_ITERATOR M_co_end,
                              const std::function<bool(state_index)>& unmarked_blocker,
                              bool& M_in_bi)
    {
      const std::size_t B_size=number_of_states_in_block(B);
      std::unordered_set<state_index> U, U_todo;
      std::unordered_set<state_index> R, R_todo;
      typedef enum { initializing, state_checking, aborted } status_type;
      status_type U_status=initializing;
      status_type R_status=initializing;
      MARKED_STATE_ITERATOR M_it=M_begin; 
      UNMARKED_STATE_ITERATOR M_co_it=M_co_begin; 
      const state_index bottom_state_walker_end=m_blocks[B].start_bottom_states;

      // Algorithm 3, line 3.2 left.
      std::unordered_map<state_index, size_t> count;


      // start coroutines. Each co-routine handles one state, and then gives control
      // to the other co-routine. The coroutines can be found sequentially below surrounded
      // by a while loop.

      while (true)
      {
        // The code for the left co-routine. 
        switch (U_status) 
        {
          case initializing:
          {
            // Algorithm 3, line 3.3 left.
            if (M_co_it==M_co_end)
            {
              U_status=state_checking;
            }
            else
            {
              const state_index si=state_in_blocks(*M_co_it);
              if (!unmarked_blocker(si)) 
              {
                U_todo.insert(si);
              }
            }
            break;
          }
          case state_checking:
          {
            // Algorithm 3, line 3.22 and line 3.23. 
            if (U_todo.empty())
            {
              assert(!U.empty());
              // split_block B into U and B\U.
              block_index block_index_of_U=split_block_B_into_R_and_BminR(B, U);
              M_in_bi = false;
              return block_index_of_U;
            }
            else
            {
              const state_index s=U_todo.extract(U_todo.begin());
              U.insert(s); 
              count(s)=0;
              // Algorithm 3, line 3.8.
              for(transition_index t=m_states[s].start_outgoing_inert_transitions;
                       t<m_states[s].start_non_inert_outgoing_transitions;
                  t++)
              {
                // Algorithm 3, line 3.11.
                state_index from=m_aut.get_transitions()[t];
                if (count.find(from)==count.end()) // count(from) is undefined;
                {
                   // Algorithm 3, line 3.12.
                  if (unmarked_blocker(from)>0)
                  {
                    // Algorithm 3, line 3.13.
                    count[from]=std::numeric_limits<std::size_t>::max;
                  }
                  else
                  {
                    // Algorithm 3, line 3.14 and 3.17.
                    count[from]=m_states[from].start_non_inert_outgoing_transitions-
                               m_states[from].start_outgoing_inert_transitions-1;
                  }
                }
                else
                {
                  // Algorithm 3, line 3.17.
                  count[from]=m_states[from]--;
                }
                // Algorithm 3, line 3.18.
                if (count[from]==0)
                {
                  if (U.count(from)==U.end())
                  {
                    U_todo.insert(from);
                  }
                }
              }
            }
            // Algorithm 3, line 3.9 and line 3.10 left. 
            if (2*(U.size()+U_todo.size())>B_size)
            {
              U_status=aborted;
            }
          }
          default: break;
        }
        // The code for the right co-routine. 
        switch (R_status)
        {
          case initializing:
          {
            // Algorithm 3, line 3.3 right.
            if (M_it==M_end)
            {
              R_status=state_checking;
            }
            else
            {
              const state_index si=state_in_blocks(*M_it);
              if (!marked_blocker(si)) 
              {
                R_todo.insert(si);
              }
            }
            break;
          }
          case state_checking: 
          {
            if (R_todo.empty())
            {
              // split_block B into R and B\R.
              block_index block_index_of_R=split_block_B_into_R_and_BminR(B, R);
              M_in_bi=true;
              return block_index_of_R;
            }
            else
            {
              const state_index s=R_todo.extract(R_todo.begin());
              R.insert(s);
              for(transition_index t=m_states[s].incoming_transitions; 
                          m_aut.is_tau(m_aut.get_transitions()[t].label()) && 
                          m_states[m_aut.get_transitions()[t].from()].block==m_states[m_aut.get_transitions()[t].to()].block; 
                  t++) 
              { 
                const transition& tr=m_aut.get_transitions()[t];
                if (R.count(tr.from())==0)
                {
                  R.todo.insert(tr.from());
                }
              }
              // Algorithm 3, line 3.9 and line 3.10 Right. 
              if (2*(R.size()+R_todo.size())>B_size)
              {
                R_status=aborted;
              }
            }
          }
          default: break;
        }
      }
    }

    // Split block B in R, being the inert-tau transitive closure of M minus those in marked_blocker contains 
    // states that must be in block, and M\R. M_nonmarked, minus those in unmarked_blocker, are those in the other block. 
    // The splitting is done in time O(min(|R|,|B\R|). Return the block index of the smallest
    // block, which is newly created. Indicate in M_in_new_block whether this new block contains M.
    template <class MARKED_STATE_ITERATOR,
              class UNMARKED_STATE_ITERATOR>
    block_index splitB(const block_index B, 
                       const MARKED_STATE_ITERATOR M_begin, 
                       const MARKED_STATE_ITERATOR M_end, 
                       const std::function<bool(state_index)>& marked_blocker,
                       const UNMARKED_STATE_ITERATOR M_co_begin,
                       const UNMARKED_STATE_ITERATOR M_co_end,
                       const std::function<bool(state_index)>& unmarked_blocker,
                       bool& M_in_new_block)
    {
      assert(M_begin!=M_end && M_co_begin!=M_co_end);
      block_index bi=simple_splitB(B, M_begin, M_end, marked_blocker, M_co_begin, M_co_end, unmarked_blocker, M_in_new_block);


      // Update the L_B_C_list, and bottom states, and invariant on inert transitions.
      for(typename std::vector<state_index>::iterator si=m_blocks[bi].start_bottom_states;
                 si!=m_states_in_blocks.end() &&
                 (bi+1==m_blocks.size() || si!=m_blocks[bi+1].start_bottom_states);
                 ++si)
      {     
        state_type_gj& s= m_states[si];
        s.block=bi;

        bool no_inert_transition_seen=true;
        
        // Adapt the L_B_C_list.
        std::unordered_set< std::pair <action_index, constellation_index>, 
                            std::forward_list< transition_index >::iterator> new_LBC_list_entries;
        for(std::vector<transition_index>::iterator ti=s.start_outgoing_inert_transitions; 
                    ti!=s.start_outgoing_non_inert_transitions;
                )
        {       
          // Situation below is only relevant if not M_in_new_block;
          if (!M_in_new_block)
          {
            const transition& t=m_aut.get_transitions()[*ti];
            assert(m_aut.is_tau(t.label()));
            if (m_states[t.from()].block()!=m_states[t.to()].block())
            {
              // This is a transition that has become non-inert.
              // Swap this transition to the non-inert transitions.
              transition_index temp=m_outgoing_transitions[s.start_outgoing_non_inert_transitions];
              m_outgoing_transitions[s.start_outgoing_non_inert_transitions]= *ti;
              *ti=temp;
              s.start_outgoing_non_inert_transitions--;
              // Do not increment ti as it refers to a new transition. 
            }
            else
            {
              update_the_doubly_linked_list_L_B_C_new_block(B, bi, *ti, m_transitions[*ti], new_LBC_list_entries);
              no_inert_transition_seen=false;
              ti++;
            }
          }
        }

        for(std::vector<transition_index>::iterator ti=s.start_outgoing_non_inert_transitions; 
                    m_aut.is_tau(m_aut.get_transitions()[ti].label()) &&
                    ti!=m_outgoing_transitions.end() &&
                    ti!=(si+1)->get().start_outgoing_transitions;
                ti++)
        {       
          update_the_doubly_linked_list_L_B_C_new_block(B, bi, *ti, m_transitions[*ti], new_LBC_list_entries);
        }
        
        if (no_inert_transition_seen)
        {
          // The state at si has become a bottom_state.
          m_P.insert(*si);
          // Move this former non bottom state to the bottom states.
          state_index temp=*si;
          *si=*(m_blocks[bi].start_non_bottom_states);
          *(m_blocks[bi].start_non_bottom_states)=temp;
          m_blocks[bi].start_non_bottom_states++;
        }

        // Investigate the incoming tau transitions. 
        if (M_in_new_block)
        {
          std::vector<std::vector<transition_index>::iterator> transitions_that_became_non_inert;
          std::vector<transition_index>::iterator last_former_inert_transition;
          for(std::vector<transition_index>::iterator ti=s.start_incoming_transitions; 
                      m_aut.is_tau(m_aut.get_transitions()[ti].label()) &&
                      ti!=m_incoming_transitions.end() &&
                      ti!=(si+1)->get().start_incoming_transitions;
                  ti++)
          {       
            if (m_states[m_aut.get_transitions()[*ti].from()].block==
                m_states[m_aut.get_transitions()[*ti].to()].block)
            {
              last_former_inert_transition=ti;
            }
            if (m_states[m_aut.get_transitions()[*ti].from()].block==B &&
                m_states[m_aut.get_transitions()[*ti].to()].block==bi)
            { 
              last_former_inert_transition=ti;
              // This transition did become non-inert.
              transitions_that_became_non_inert.push_back(ti);
              // Check whether from is a new bottom state.
              state_index from=m_aut.get_transitions()[*ti].from();
              transition& from_trans= m_aut.transitions(*m_states[from].outgoing_transitions);
              assert(from_trans.from()==from);
              if (m_states[from].block!=m_states[from_trans.to()].block ||
                  !m_aut.is_tau(from_trans.label()))
              {
                // This transition is not inert. The state from is a new bottom state.
                m_P.insert(from);
                // Move this former non bottom state to the bottom states.
                typename std::vector<state_index>::iterator position_in_states_in_blocks=m_states[from].ref_states_in_blocks;
                state_index temp=*position_in_states_in_blocks;
                block_index temp_bi=m_states[from].block;
                *position_in_states_in_blocks=*(m_blocks[temp_bi].start_non_bottom_states);
                *(m_blocks[temp_bi].start_non_bottom_states)=temp;
                m_blocks[temp_bi].start_non_bottom_states++;
              }
            }
          }
          // Move the non_inert_transitions to the end.
          while(!transitions_that_became_non_inert.empty())
          {
            std::vector<transition_index>::iterator tti=transitions_that_became_non_inert.back();
            transitions_that_became_non_inert.pop_back();
            transition_index temp= *tti;
            *tti=*last_former_inert_transition;
            *last_former_inert_transition=temp;
          }
        }
      }

      return bi;
    }


    void create_initial_partition()
    {
      mCRL2log(log::verbose) << "An O(m log n) "
           << (m_branching ? (m_preserve_divergence
                                         ? "divergence-preserving branching "
                                         : "branching ")
                         : "")
           << "bisimulation partitioner created for " << m_aut.num_states()
           << " states and " << m_aut.num_transitions() << " transitions.\n";
      // Initialisation.

      // Initialise m_incoming_transitions and m_transitions.trans_count and m_transitions.transitions_per_block_to_constellation.
      typedef std::unordered_multimap<typename std::pair<typename LTS_TYPE::states_size_type, typename LTS_TYPE::labels_size_type>, 
                                      transition&> temporary_store_type;
      temporary_store_type temporary_store;
      for(const transition& t: m_aut.get_transitions())
      {
        temporary_store[std::pair(t.from(),t.label())]=t;
      }
      state_index current_from_state=-1;
      label_index current_label=-1;
      m_incoming_transitions.reserve(m_aut.num_transitions());
      for(auto [_,t]: temporary_store)
      {
        m_incoming_transitions.emplace(t);
        if (t.from()!=current_from_state || t.label()!=current_label)
        {
          m_state_to_constellation_count.emplace_back(1);
          m_blocks[0].block_to_constellation.push_front(t);
          current_label=t.label();
        }
        else
        {
          assert(m_state_to_constellation_count.size()>0);
          m_state_to_constellation_count.back()++;
          m_blocks[0].block_to_constellation.front().push_front(t);
        }
        if (t.from()!=current_from_state)
        {
          m_states[t.from()].start_incoming_transitions=m_incoming_transitions.end()-1;
          current_from_state=t.from();
        }
        std::size_t transition_index=std::distance(t,m_aut.get_transitions().begin());
        m_transitions[transition_index].trans_count=m_state_to_constellation_count.end()-1;
        m_transitions[transition_index].transitions_per_block_to_constellation=m_blocks[0].block_to_constellation.front().begin();
      }
      temporary_store.clear();
      
      // Initialise m_outgoing_transitions and
      // initialise m_states_in_blocks, together with start_bottom_states start_non_bottom_states in m_blocks.
      std::vector<bool> state_has_outgoing_tau(m_aut.num_states(),false);
      for(const transition& t: m_aut.get_transitions())
      {
        temporary_store[std::pair(t.to(),t.label())]=t;
        if (m_aut.is_tau(t))
        {
          state_has_outgoing_tau[t.from()]=true;
        }
      }
      m_outgoing_transitions.reserve(m_aut.num_transitions());
      typename LTS_TYPE::states_sizes_type current_to_state=-1;
      for(auto [_,t]: temporary_store)
      {
        m_outgoing_transitions.emplace(t);
        if (t.to()!=current_to_state)
        {
          m_states[t.to()].start_outgoing_inert_transitions=m_outgoing_transitions.end()-1;
          current_to_state=t.to();
        }
      }
      temporary_store=temporary_store_type(); // release memory. 

      m_states_in_blocks.reserve(m_aut.num_states());
      std::size_t i=0;
      for(bool b: state_has_outgoing_tau)
      {
        if (b)
        {
          m_states_in_blocks.emplace_back(i);
        }
        i++;
      }
      m_blocks[0].start_bottom_states=0;
      m_blocks[0].start_non_bottom_states=i;
      i=0;
      for(bool b: state_has_outgoing_tau)
      {
        if (!b)
        {
          m_states_in_blocks.emplace_back(i);
        }
        i++;
      }

      // The data structures are now initialized.
      // The following implements line 1.3 of Algorithm 1. 
      states_per_action_label_type states_per_action_label;
      for(const transition& t: m_aut.get_transitions())
      {
        states_per_action_label[t.label()]=states_per_action_label[t.label()].insert(t.from());
      }

      for(const set_of_states_type& stateset: states_per_action_label)
      {
        for(const set_of_states_type& M: stateset)
        {
          states_per_block_type Bprime;
          for(const state_index s: M)
          {
            Bprime[m_states[s].block].insert(s);
          }
          
          for(auto [block_index, split_states]: Bprime)
          {
            // Check whether the block B, indexed by block_index, can be split.
            // This means that the bottom states of B are not all in the split_states.
            const block_type& B=m_blocks[block_index];
            for(state_index i=B.start_bottom_states; i<B.start_non_bottom_states; ++i)
            {
              if (!split_states.contains(i))
              {
                simpleSplitB(block_index, split_states);
                i=B.start_non_bottom_states; // This means break the loop.
              }
            }
          }
        }
      }

      std::unordered_map<block_index, set_of_states_type> P;
      check_transitions_that_became_non_inert(P);
    }
 
    // Update the doubly linked list L_B->C in blocks.
    // First removing and adding a single element are implemented.
    //
    // If there is more than one element it is removed.
    // In that case false is returned. Otherwise, the result is true, 
    // and the element is actually not removed.
    bool remove_from_the_doubly_linked_list_L_B_C_in_blocks(const transition_index ti)
    {
      if (m_transitions[ti].previous_L_B_C_element==null_transition &&
          m_transitions[ti].previous_L_B_C_element==null_transition)
      {
        // This is the only element in the list. Leave it alone.
        return true;
      }
      else
      {
        // There is more than one element.
        if (m_transitions[ti].previous_L_B_C_element!=null_transition)
        {
          m_transitions[m_transitions[ti].previous_L_B_C_element].next_L_B_C_element=
              m_transitions[ti].next_L_B_C_element;
        }
        if (m_transitions[ti].next_L_B_C_element!=null_transition)
        {
          m_transitions[m_transitions[ti].next_L_B_C_element].previous_L_B_C_element=
              m_transitions[ti].previous_L_B_C_element;
        }
        return false;
      }
    }


    // Algorithm 4. Stabilize the current partition with respect to the current constellation
    // given that the states in m_P did become new bottom states. 

    void stabilizeB()
    {
      // Algorithm 4, line 4.3.
      std::unordered_map<block_index, set_of_states_type> Phat;
      for(const state_index si: m_P)
      {
        Phat[m_states[si].block].insert(si);
      }
      m_P.clear();

      // Algorithm 4, line 4.4.
      while (!Phat.empty()) 
      {
        // Algorithm 4, line 4.5. 
        const block_index bi=Phat.front().first;
        const set_of_states_type& V=Phat.front().second;
        Phat.erase(bi);
        
        // Algorithm 4, line 4.6.
        // Collect all new bottom states and group them per action-constellation pair.
        std::unordered_map<std::pair<action_index, constellation_index>, state_index> grouped_transitions;
        for(const state_index si: V)
        {
          for(transition_index ti=m_states[si].outgoing_transitions;
                        ti!=m_outgoing_transitions.size() &&
                        (si+1>=m_states.size() || ti!=m_states[si+1].outgoing_transitions);
                    ++ti)
          {
            transition& t=m_aut.get_transitions()[ti];
            grouped_transitions[std::pair(t.label(), m_blocks[m_states[t.to()].block()].constellation())]=t.from();
          }
        }
        
        // Algorithm 4, line 4.7, and implicitly 4.8.
        for(std::forward_list< transition_index >::iterator Q=m_blocks[bi].block_to_constellation.begin();
                      Q!=m_blocks[bi].block_to_constellation.end(); ++Q)
        {
          // Algorithm 4, line 4.9.
          set_of_states_type W=V;
          transition& t=m_aut.get_transitions()[*Q];
          set_of_states_type& aux=grouped_transitions[std::pair(t.label(), m_blocks[m_states[t.to()].block()].constellation())];
          W.erase(aux.begin(),aux.end());
          // Algorithm 4, line 4.10.
          if (!W.empty())
          {
            // Algorithm 4, line 4.11, and implicitly 4.12. 
            bool V_in_bi=false;
            block_index bi_new=splitB(bi, 
                                      m_blocks[bi].start_bottom_states, 
                                      m_blocks[bi].start_non_bottom_states,
                                      [&W](const state_index si){ return W.count(si)!=0; },
                                      W.begin(), 
                                      W.end(),
                                      [](const state_index ){ return false; },
                                      V_in_bi);
            // Algorithm 4, line 4.13.
            if (V_in_bi)
            {
              // The new bottom states V are in block bi, and block bi is the largest block of bi and bi_new.
              check_transitions_that_became_non_inert_in_a_block_by_co_block(bi, bi_new);
            }
            else
            {
              // The new bottom states V are in block bi_new, and block bi is the largest block of bi and bi_new.
              check_transitions_that_became_non_inert_in_a_block(bi_new);
            }
            
          }
        }
        
        // Algorithm 4, line 4.17.
        for(const state_index si: m_P)
        {
          Phat[m_states[si].block].insert(si);
        }
        m_P.clear();

      }
    }

    void refine_partition_until_it_becomes_stable()
    {
      // This represents the while loop in Algorithm 1 from line 1.6 to 1.25.

      // Algorithm 1, line 1.6.
      while (!m_non_trivial_constellations.empty())
      {
        const set_of_constellations_type::const_iterator i=m_non_trivial_constellations.begin();
        constellation_index ci= *i;
        m_non_trivial_constellations.extract(i);

        // Algorithm 1, line 1.7.
        std::size_t index_block_B=m_constellations[ci].start_block;
        if (number_of_states_in_block(index_block_B)<number_of_states_in_block(index_block_B+1))
        {
          m_states_in_blocks[index_block_B].swap(m_states_in_blocks[index_block_B+1]);
        }
        
        // Algorithm 1, line 1.8.
        m_constellations[ci].start_block=index_block_B+1;
        if (m_constellations[ci].start_block!=m_constellations[ci].end_block) // Constellation is not trivial.
        {
          m_non_trivial_constellations.insert(ci);
        }
        m_constellations.emplace_back(index_block_B, index_block_B);
        // Here the variables block_to_constellation and the doubly linked list L_B->C in blocks must be still be updated.
        // Moreover, the variable m_state_to_constellation_count in transitions requires updating.
        // This happens below.

        // Algorithm 1, line 1.9.
        states_per_action_label_type calM;
        state_label_to_size_t_map newly_created_state_to_constellation_count_entry;
        label_to_size_t_map label_to_transition;
        state_label_to_size_t_map outgoing_transitions_count_per_label_state;
        
 
        // Walk through all states in block B
        for(typename std::vector<state_index>::iterator i=m_states_in_blocks[index_block_B].start_bottom_states;
                i!=m_states_in_blocks.end() &&
                (index_block_B+1>m_blocks.size() || i!=m_states_in_blocks[index_block_B+1].start_bottom_states); 
               ++i)
        {
          // and visit the incoming transitions. 
          for(std::vector<transition_index>::iterator j=i->get().start_incoming_transitions;
                 j!=m_incoming_transitions.end() &&
                 (i+1!=m_states_in_blocks.end() || j!=(i+1)->get().start_incoming_transitions); 
                ++j)
          {
            const transition& t=m_aut.transitions[*j];
            // Add the source state grouped per label in calM, provided the transition is non inert.
            if (!m_aut.is_tau(t.label()) || m_states[t.from()].m_block!=m_states[t.to()].m_block)
            {
              calM[t.label()].insert(t.from());
            }
            // Update outgoing_transitions_count_per_label_state;
            typename state_label_to_size_t_map::iterator it=outgoing_transitions_count_per_label_state.find(std::pair(t.label(),t.from()));
            if (it==outgoing_transitions_count_per_label_state.end())
            {
              outgoing_transitions_count_per_label_state[std::pair(t.label(),t.from())](*m_transitions[*j].trans_count)-1;
            }
            else 
            {
              *it--; 
            }
            // Update m_state_to_constellation_count.
            if (m_states[t.to()].block==index_block_B)
            {
              const std::size_t new_position=m_state_to_constellation_count.size();
              const std::size_t found_position=
                                   newly_created_state_to_constellation_count_entry.try_emplace(
                                                                      std::pair(t.from(),t.label()),
                                                                      new_position)->second;
              if (new_position==found_position)
              {
                m_state_to_constellation_count.push_back(1);
                (m_transitions[*j].trans_count)--;
                m_transitions[*j].trans_count=m_state_to_constellation_count.end()-1;
              }
            }
            // Update the label_to_transition map.
            if (label_to_transition.find(std::pair(t.from(), t.label()))==label_to_transition.end())
            {
              // Not found. Add a transition from the L_B_C_list to label_to_transition
              // that goes to C\B, or the null_transition if no such transition exists, which prevents searching
              // the list again. 
              // First look backwards.
              L_B_C_list_iterator transition_walker(*j, m_transitions);

              bool found=false;
              
              while (!found && i*transition_walker!=null_transition)
              {
                transition& tw=m_aut.get_transitions()[transition_walker];
                if (m_states[tw.to()].block==ci)
                {
                  found=true;
                }
                else
                {
                  ++transition_walker;
                }
              }
              label_to_transition[t.label()]=transition_walker;
            }
            // Update the doubly linked list L_B->C in blocks as the constellation is split in B and C\B. 
            update_the_doubly_linked_list_L_B_C_same_block(index_block_B, t, *j);
          }
        }
        
        // Algorithm 1, line 1.10.
        for(const auto& [a, M]: calM)
        {
          // Algorithm 1, line 1.11.
          states_per_block_type Mleft_map;
          for(const state_index si: M)
          {
            Mleft_map[m_states[si].block].insert(si);
          }
          for(const auto& [bi, Mleft]: Mleft_map)
          {
            assert(!Mleft.empty());
            // Check whether the bottom states of bi are not all included in Mleft. 
            bool bottom_states_all_included=true;
            for(typename std::vector<state_index>::iterator state_it=m_blocks[bi].start_bottom_states;
                      state_it!=m_blocks[bi].start_non_bottom_states;
                    state_it++)
            {
              if (Mleft.count(*state_it)==0)
              {
                bottom_states_all_included=false;
                break; // leave the for loop.
              }
            }
            if (!bottom_states_all_included)
            {
              // Algorithm 1, line 1.12.
              bool M_in_bi1=true;
              block_index bi1=splitB(bi, Mleft.begin(), 
                                         Mleft.end(), 
                                         [](const state_index ){ return false; },
                                         m_blocks[bi].start_bottom_states, 
                                         m_blocks[bi].start_non_bottom_states, 
                                         [&](const state_index si){ return Mleft.count(si)>0; },
                                         M_in_bi1);
              // Algorithm 1, line 1.13.
              block_index Bpp=(M_in_bi1, bi1, bi);
              // Algorithm 1, line 1.14 is implicitly done in the call of splitB above.
              // Algorithm 1, line 1.15.
              bool size_U_larger_than_zero=false;
              bool size_U_smaller_than_Bpp_bottom=false;
              for(state_index si=m_states[Bpp].bottom_states;
                                !(size_U_larger_than_zero && size_U_smaller_than_Bpp_bottom) &&
                                si!=m_states[Bpp].non_bottom_states; 
                              ++si)
              {
                if (outgoing_transitions_count_per_label_state[std::pair(a,si)]>0)
                {
                  size_U_larger_than_zero=true;
                }
                else
                {
                  size_U_larger_than_zero=true;
                }
              }
              // Algorithm 1, line 1.16.
              if (size_U_larger_than_zero && size_U_smaller_than_Bpp_bottom) 
              {
                // Algorithm 1, line 1.17.
                transition_index co_t=label_to_transition[a];
                
                bool dummy=false;
                splitB(Bpp, L_B_C_list_iterator(co_t,m_transitions), 
                            L_B_C_list_iterator(null_transition, m_transitions), 
                            [](const state_index ){ return false; },
                            m_states[Bpp].bottom_states, 
                            m_states[Bpp].non_bottom_states,
                            [&](const state_index si){ outgoing_transitions_count_per_label_state.count(std::pair(a,si))>0; },
                            dummy);
                // Algorithm 1, line 1.18 and 1.19. P is updated implicitly when splitting Bpp.

              }
              
            }
          }
        }
        stabilizeB();
      }
    }

};




/* ************************************************************************* */
/*                                                                           */
/*                             I N T E R F A C E                             */
/*                                                                           */
/* ************************************************************************* */





/// \defgroup part_interface
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
    // Line 2.1: Find tau-SCCs and contract each of them to a single state
    if (branching)
    {
        scc_reduce(l, preserve_divergence);
    }

    // Now apply the branching bisimulation reduction algorithm.  If there
    // are no taus, this will automatically yield strong bisimulation.
    bisim_partitioner_gj<LTS_TYPE> bisim_part(l, branching,
                                                          preserve_divergence);

    // Assign the reduced LTS
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
        mCRL2log(log::warning) << "The JGKW20 branching bisimulation "
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
    bisim_partitioner_gj<LTS_TYPE> bisim_part(l1, branching,
                                                          preserve_divergence);

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