// Author(s): Jan Friso Groote and David N. Jansen
//
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file lts/detail/liblts_bisim_gj_lazy_BLC.h
///
/// \brief O(m log n)-time branching bisimulation algorithm similar to liblts_bisim_dnj.h
///        which does not use bunches, i.e., partitions of transitions. This algorithm
///        should be slightly faster, but in particular use less memory than liblts_bisim_dnj.h.
///        Otherwise the functionality is exactly the same. This file is equal to liblts_bisim_gj.h
///        with as additional feature that it tries to only build BLC sets when needed. This is
///        only needed when stabilizing large blocks with new bottom states. For instance, for 
///        strong bisimulation such states do not occur, and no BLC sets need to be constructed. 

#ifndef LIBLTS_BISIM_GJ_LAZY_BLC_H
#define LIBLTS_BISIM_GJ_LAZY_BLC_H

#include <iomanip> // for std::fixed, std::setprecision(), std::setw()
#include <ctime> // for std::clock_t, std::clock()
#include <forward_list> // for std::forward_list
#include <unordered_set> // for std::unordered_set
#include "mcrl2/lts/detail/liblts_scc.h"
#include "mcrl2/lts/detail/liblts_merge.h"
#include "mcrl2/lts/detail/check_complexity.h"
#include "mcrl2/lts/detail/fixed_vector.h"
#include "mcrl2/lts/detail/simple_list.h"

#include "mcrl2/lts/detail/liblts_bisim_gj.h" // for a few definitions that are unchanged at the beginning of this file

namespace mcrl2::lts::detail
{

template <class LTS_TYPE> class bisim_partitioner_gj_lazy_BLC;

namespace bisimulation_gj_lazy_BLC
{

// Forward declaration.
struct state_type_gj_lb;
struct block_type_lb;
struct BLC_source_type;
struct block_that_needs_refinement_type;
struct constellation_type_lb;
struct transition_type_lb;
struct outgoing_transition_type_lb;

using outgoing_transitions_it_lb = fixed_vector<outgoing_transition_type_lb>::iterator;
using outgoing_transitions_const_it_lb = fixed_vector<outgoing_transition_type_lb>::const_iterator;

constexpr constellation_type_lb* null_constellation_lb=nullptr;
constexpr block_type_lb* null_block_lb=nullptr;

/* The following definitions are the same as in liblts_bisim_gj.h:
using state_index = std::size_t;
using transition_index = std::size_t;
using label_index = std::size_t;

constexpr transition_index null_transition=-1;
constexpr label_index null_action=-1;
constexpr state_index null_state=-1;

/// default counter value if the counter field of a state is not in use currently
constexpr transition_index undefined=0;

  /// \brief the number of counter values that can be used for one subblock
  /// \details There are three singular values (`undefined`, `marked_NewBotSt`,
  /// and `marked_HitSmall`), and the other values need to be distributed over
  /// three subblocks (ReachAlw, AvoidLrg, and AvoidSml).
  constexpr transition_index marked_range=
    (std::numeric_limits<transition_index>::max()-2)/3;

  enum subblocks { ReachAlw=0,// states that can reach always all splitters
                   AvoidSml,  // states that cannot inertly reach the small
                              // splitter (while it is not empty)
                   AvoidLrg,  // states that cannot inertly reach the
                              // large splitter (while it is not empty)
                   NewBotSt}; // states that can inertly reach multiple of
                              // the above subblocks
                // The following values are used only for temporary marking
                // and are not really associated with a subblock:
                // HitSmall -- states that can (non-inertly) reach the small
                //             splitter; they can be in any subblock except
                //             AvoidSml.  Necessary for correctness.

  /// \brief base marking value for a subblock
  /// \details If the counter has this value, the state definitely belongs
  /// to the respective subblock.
  static inline constexpr transition_index marked(enum subblocks subblock)
  {
    return                                                                      assert(ReachAlw==subblock || AvoidSml==subblock ||
                                                                                       AvoidLrg==subblock || NewBotSt==subblock),
           marked_range*subblock+1;
  }

  /// counter value to indicate that a state is in the NewBotSt subset
  constexpr transition_index marked_NewBotSt=marked(NewBotSt);                  static_assert(marked_NewBotSt<std::numeric_limits<transition_index>::max());

  /// counter value to indicate that a state has a transition in the small
  ///splitter (so it cannot become part of AvoidSml)
  constexpr transition_index marked_HitSmall=marked_NewBotSt+1;

  /// \brief checks whether a counter value is a marking for a given subblock
  static inline constexpr bool is_in_marked_range_of
                            (transition_index counter, enum subblocks subblock)
  {
    return                                                                      assert(ReachAlw==subblock || AvoidSml==subblock || AvoidLrg==subblock),
           counter-marked(subblock)<marked_range;
  }

/// The function clear() takes care that a container frees memory when it is
/// cleared and it is large.
template <class CONTAINER>
static inline void clear(CONTAINER& c)
{
  if (c.size()>1000) { c=CONTAINER(); } else { c.clear(); }
}

// The struct below facilitates to walk through a LBC_list starting from an
// arbitrary transition.
using BLC_list_iterator = transition_index*;             // should not be nullptr
using BLC_list_iterator_or_null = transition_index*;     // can be nullptr
using BLC_list_const_iterator = const transition_index*; // should not be nullptr
*/

/// information about a transition stored in m_outgoing_transitions
struct outgoing_transition_type_lb
{
  /// pointer to the corresponding entry in m_BLC_transitions
  BLC_list_iterator ref_BLC_transitions;

  /// this pointer is used to find transitions with the same source state, action label, and target constellation
  /// (Transitions are grouped according to these in m_outgoing_transitions.)
  /// For most transitions, it points to the last transition with the same source state, action label, and target constellation;
  /// but if this transition is the last one in the group, start_same_saC points to the first transition in the group.
  outgoing_transitions_it_lb start_same_saC;

  // The default initialiser does not initialize the fields of this struct.
  outgoing_transition_type_lb()
  {}

  outgoing_transition_type_lb(const outgoing_transitions_it_lb sssaC)
   : start_same_saC(sssaC)
  {}
};

/// a pointer to a state, i.e. a reference to a state
struct state_in_block_pointer_lb
{
  state_in_block_pointer_lb(fixed_vector<state_type_gj_lb>::iterator new_ref_state)
   : ref_state(new_ref_state)
  {}

  state_in_block_pointer_lb()
  {}

  fixed_vector<state_type_gj_lb>::iterator ref_state;

  bool operator==(const state_in_block_pointer_lb other) const
  {
    return ref_state==other.ref_state;
  }

  bool operator!=(const state_in_block_pointer_lb other) const
  {
    return ref_state!=other.ref_state;
  }
};

/// a vector with an additional (internal) field to indicate how much work has been
/// done already on it.
class todo_state_vector_lb
{
  std::size_t m_todo_indicator=0;
  std::vector<state_in_block_pointer_lb> m_vec;

  public:
    using const_iterator =
                        std::vector<state_in_block_pointer_lb>::const_iterator;
                                                                                #ifndef NDEBUG
                                                                                  bool find(const state_in_block_pointer_lb s) const
                                                                                  {
                                                                                    return std::find(m_vec.begin(), m_vec.end(), s)!=m_vec.end();
                                                                                  }
                                                                                #endif
    void add_todo(const state_in_block_pointer_lb s)
    {                                                                           assert(!find(s));
      m_vec.push_back(s);
    }

    std::size_t todo_is_empty() const
    {
      return m_vec.size()==m_todo_indicator;
    }

    // Move a state from the todo part to the definitive vector.
    state_in_block_pointer_lb move_from_todo()
    {                                                                           assert(!todo_is_empty());
      state_in_block_pointer_lb result=m_vec[m_todo_indicator];
      m_todo_indicator++;
      return result;
    }

    void swap_vec(std::vector<state_in_block_pointer_lb>& other_vec)
    {                                                                           assert(empty());  assert(0==m_todo_indicator);
      m_vec = std::move(other_vec);
      other_vec.clear();
    }

    std::size_t size() const
    {
      return m_vec.size();
    }

    std::size_t empty() const
    {
      return m_vec.empty();
    }

    const_iterator begin() const
    {
      return m_vec.begin();
    }

    const_iterator end() const
    {
      return m_vec.end();
    }

    const state_in_block_pointer_lb* data() const
    {
      return m_vec.data();
    }

    const state_in_block_pointer_lb* data_end() const
    {
      return m_vec.data() + m_vec.size();
    }

    const state_in_block_pointer_lb& front() const
    {
      return m_vec.front();
    }

    //const state_in_block_pointer_lb& back() const
    //{
    //  return m_vec.back();
    //}

    void reserve(std::vector<state_in_block_pointer_lb>::size_type new_cap)
    {
      m_vec.reserve(new_cap);
    }

    using iterator = std::vector<state_in_block_pointer_lb>::iterator;

    iterator begin()
    {
      return m_vec.begin();
    }

    iterator end()
    {
      return m_vec.end();
    }

    // add all elements in [begin, end) to the vector
    void add_todo(iterator begin, iterator end)
    {
      m_vec.insert(m_vec.end(), begin, end);
    }

    void clear()
    {
      m_todo_indicator=0;
      bisimulation_gj::clear(m_vec);
    }
};



// Below the four main data structures are listed.
/// information about a state
struct state_type_gj_lb
{
  /// block of the state
  block_type_lb* block = null_block_lb;
  /// first incoming transition
  std::vector<transition>::iterator start_incoming_transitions;
  /// first outgoing transition
  outgoing_transitions_it_lb start_outgoing_transitions;
  /// pointer to the corresponding entry in m_states_in_blocks
  state_in_block_pointer_lb* ref_states_in_blocks = nullptr;
  /// number of outgoing block-inert transitions
  transition_index no_of_outgoing_block_inert_transitions=0;
  /// counter used during splitting
  /// If this counter is set to undefined (0), it is considered to be not yet
  /// visited.
  /// If this counter is a positive number, it is the number of outgoing
  /// block-inert transitions that have not yet been handled.
  transition_index counter=undefined;
                                                                                #ifndef NDEBUG
                                                                                  /// \brief print a short state identification for debugging
                                                                                  template<class LTS_TYPE>
                                                                                  std::string debug_id_short(const bisim_partitioner_gj_lazy_BLC<LTS_TYPE>& partitioner) const
                                                                                  {
                                                                                    assert(partitioner.m_states.data()<=this);
                                                                                    assert(this<partitioner.m_states.data_end());
                                                                                    return std::to_string(this-partitioner.m_states.data());
                                                                                  }

                                                                                  /// \brief print a state identification for debugging
                                                                                  template<class LTS_TYPE>
                                                                                  std::string debug_id(const bisim_partitioner_gj_lazy_BLC<LTS_TYPE>& partitioner) const
                                                                                  {
                                                                                    return "state " + debug_id_short(partitioner);
                                                                                  }
                                                                                #endif
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  mutable check_complexity::state_gj_counter_t work_counter;
                                                                                #endif
};

/// The following type gives the start and end indications of the transitions
/// for the same superblock, label and constellation in the array
/// m_BLC_transitions.
struct BLC_indicators_lb
{
  BLC_list_iterator start_same_BLC;

  // If the source block of the BLC_indicator has new bottom states,
  // it is undefined whether the BLC_indicator should be regarded as stable or
  // unstable. Otherwise, the BLC_indicator is regarded as stable if and only
  // if start_marked_BLC is ==nullptr.
  BLC_list_iterator_or_null start_marked_BLC;
  BLC_list_iterator end_same_BLC;
  /// \brief is true if it is known that the super-BLC set transitions start in a small subblock
  /// \details (Perhaps memory could be saved by adding a second boolean
  /// `is_stable` and suppressing `end_same_BLC`.  For stable super-BLC sets,
  /// the end of the transitions is the same as `end_marked_BLC`.  Loops going
  /// through all transitions in a (possibly unstable) super-BLC set would have
  /// to test this:
  /// ```
  /// tr < start_marked_BLC || (!is_stable &&
  ///                           tr < m_BLC_transitions.data_end() &&
  ///                           source is in blc_src &&
  ///                           action label agrees &&
  ///                           target is in to_constln)
  /// ```
  /// .)
  bool starts_in_small_subblock = true;

  BLC_indicators_lb(BLC_list_iterator start, BLC_list_iterator end,
                    bool is_stable)
   : start_same_BLC(start),
     start_marked_BLC(is_stable ? nullptr : end),
     end_same_BLC(end)
  {                                                                             assert(nullptr!=start_same_BLC);  assert(nullptr!=end_same_BLC);
                                                                                assert(start_same_BLC<=end_same_BLC);
  }

  bool is_stable() const
  {                                                                             assert(nullptr!=start_same_BLC);  assert(nullptr!=end_same_BLC);
                                                                                assert(nullptr==start_marked_BLC || start_same_BLC<=start_marked_BLC);
                                                                                assert(nullptr==start_marked_BLC || start_marked_BLC<=end_same_BLC);
                                                                                assert(start_same_BLC<=end_same_BLC);
    return nullptr==start_marked_BLC;
  }

  /// This function returns true iff the BLC set contains at least one
  /// marked transition.
  bool has_marked_transitions() const
  {
    if (is_stable())
    {
      return false;
    }
    return start_marked_BLC<end_same_BLC;
  }

  void make_stable()
  {                                                                             assert(!is_stable());
    start_marked_BLC=nullptr;
  }

  void make_unstable()
  {                                                                             assert(is_stable());
    start_marked_BLC=end_same_BLC;
  }

  bool operator==(const BLC_indicators_lb& other) const
  {
    return start_same_BLC==other.start_same_BLC &&
           start_marked_BLC==other.start_marked_BLC &&
           end_same_BLC==other.end_same_BLC;
  }

  bool operator!=(const BLC_indicators_lb& other) const
  {
    return !operator==(other);
  }
                                                                                #ifndef NDEBUG
                                                                                  /// \brief print a B_to_C slice identification for debugging
                                                                                  /// \details This function is only available if compiled in Debug mode.
                                                                                  template<class LTS_TYPE>
                                                                                  std::string debug_id(const bisim_partitioner_gj_lazy_BLC<LTS_TYPE>& partitioner) const
                                                                                  {
                                                                                    assert(partitioner.m_BLC_transitions.data()<=start_same_BLC);
                                                                                    assert(nullptr==start_marked_BLC || start_same_BLC<=start_marked_BLC);
                                                                                    assert(nullptr==start_marked_BLC || start_marked_BLC<=end_same_BLC);
                                                                                    assert(start_same_BLC<=end_same_BLC);
                                                                                    assert(end_same_BLC<=partitioner.m_BLC_transitions.data_end());
                                                                                    std::string result("super-BLC set ["+std::to_string(std::distance<BLC_list_const_iterator>(&*partitioner.m_BLC_transitions.begin(), start_same_BLC))+","+std::to_string(std::distance<BLC_list_const_iterator>(&*partitioner.m_BLC_transitions.begin(), end_same_BLC))+")");
                                                                                    if (start_same_BLC==end_same_BLC)
                                                                                    {
                                                                                      return "Empty "+result;
                                                                                    }
                                                                                    result += " from ";
                                                                                    result += partitioner.m_states[partitioner.m_aut.get_transitions()[*start_same_BLC].from()].block->block_BLC_source->debug_id(partitioner);
                                                                                    result += " to ";
                                                                                    result += partitioner.m_states[partitioner.m_aut.get_transitions()[*start_same_BLC].to()].block->constellation->debug_id(partitioner);
                                                                                    result += " containing the ";
                                                                                    if (std::distance(start_same_BLC, end_same_BLC)>1)
                                                                                    {
                                                                                        result+=std::to_string(std::distance(start_same_BLC, end_same_BLC));
                                                                                        result += " transitions ";
                                                                                    }
                                                                                    else
                                                                                    {
                                                                                        result += "transition ";
                                                                                    }
                                                                                    BLC_list_const_iterator iter = start_same_BLC;
                                                                                    if (start_marked_BLC == iter)
                                                                                    {
                                                                                        result += "| ";
                                                                                    }
                                                                                    result += partitioner.m_transitions[*iter].debug_id_short(partitioner);
                                                                                    if (std::distance(start_same_BLC, end_same_BLC)>4)
                                                                                    {
                                                                                        ++iter;
                                                                                        result += start_marked_BLC == iter ? " | " : ", ";
                                                                                        result += partitioner.m_transitions[*iter].debug_id_short(partitioner);
                                                                                        result += std::next(iter) == start_marked_BLC ? " | ..."
                                                                                                  : (!is_stable() && start_marked_BLC>std::next(iter) && start_marked_BLC<=end_same_BLC-3 ? ", ..|.." : ", ...");
                                                                                        iter = end_same_BLC-3;
                                                                                    }
                                                                                    while (++iter!=end_same_BLC)
                                                                                    {
                                                                                        result += start_marked_BLC == iter ? " | " : ", ";
                                                                                        result += partitioner.m_transitions[*iter].debug_id_short(partitioner);
                                                                                    }
                                                                                    if (start_marked_BLC == iter)
                                                                                    {
                                                                                        result += " |";
                                                                                    }
                                                                                    return result;
                                                                                  }
                                                                                #endif
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  mutable check_complexity::BLC_gj_counter_t work_counter;
                                                                                #endif
};

/// information about a transition
/// The source, label and target of the transition are not stored here but in
/// m_aut.get_transitions(), to save memory.
struct transition_type_lb
{
  // The position of the transition type corresponds to m_aut.get_transitions().
  // std::size_t from, label, to are found in m_aut.get_transitions().
  simple_list<BLC_indicators_lb>::iterator
                                        transitions_per_block_to_constellation;
  outgoing_transitions_it_lb ref_outgoing_transitions;  // This refers to the position of this transition in m_outgoing_transitions.
                                                     // During initialisation m_outgoing_transitions contains the indices of this
                                                     // transition. After initialisation m_outgoing_transitions refers to the corresponding
                                                     // entry in m_BLC_transitions, of which the field transition contains the index
                                                     // of this transition.
                                                                                #ifndef NDEBUG
                                                                                  /// \brief print a short transition identification for debugging
                                                                                  /// \details This function is only available if compiled in Debug mode.
                                                                                  template<class LTS_TYPE> std::string debug_id_short
                                                                                             (const bisim_partitioner_gj_lazy_BLC<LTS_TYPE>& partitioner) const
                                                                                  {
                                                                                    assert(partitioner.m_transitions.data()<=this);
                                                                                    assert(this<partitioner.m_transitions.data_end());
                                                                                    const transition& t=partitioner.m_aut.get_transitions()
                                                                                                                       [this-partitioner.m_transitions.data()];
                                                                                    return partitioner.m_states[t.from()].debug_id_short(partitioner) + " -" +
                                                                                           pp(partitioner.m_aut.action_label(t.label())) + "-> " +
                                                                                           partitioner.m_states[t.to()].debug_id_short(partitioner);
                                                                                  }

                                                                                  /// \brief print a transition identification for debugging
                                                                                  /// \details This function is only available if compiled in Debug mode.
                                                                                  template<class LTS_TYPE> std::string debug_id
                                                                                             (const bisim_partitioner_gj_lazy_BLC<LTS_TYPE>& partitioner) const
                                                                                  {
                                                                                    return "transition " + debug_id_short(partitioner);
                                                                                  }
                                                                                #endif
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  mutable check_complexity::trans_gj_counter_t work_counter;
                                                                                #endif
};

/// \brief information about a block
/// \details A block is mainly described through the set of states it contains.
/// For this we have `fixed_vector<state_in_block_pointer_lb> m_states_in_blocks`,
/// where states are kept grouped by block.  The fields `start_bottom_states`,
/// `sta.rt_non_bottom_states` and `end_states` are pointers into that array.
///
/// Some fields get a second life (to save memory) during initialisation or
/// during finalising; that is the purpose of the unions.
///
/// A block should be trivially destructible because we want it to be allocated
/// using the pool allocator `simple_list<BLC_indicators_lb>::get_pool()`.  This
/// is why there are no iterator fields.
struct block_type_lb
{
  /// constellation that the block is in
  constellation_type_lb* constellation;

/*
  /// \brief first unmarked bottom state
  /// \details used in particular during initialisation, but also when a
  /// super-BLC-set with multiple source blocks is handled.
  state_in_block_pointer_lb* first_unmarked_bottom_state;
*/

  /// first state of the block in m_states_in_blocks
  /// States in [start_bottom_states, sta.rt_non_bottom_states) are bottom
  /// states in the block
  state_in_block_pointer_lb* start_bottom_states;

  union start_non_bottom_states_or_state_in_reduced_LTS
  {
    /// first non-bottom state of the block in m_states_in_blocks
    /// States in [sta.rt_non_bottom_states, end_states) are non-bottom states
    /// in the block.
    ///
    /// If m_branching==false, we have sta.rt_non_bottom_states==end_states.
    state_in_block_pointer_lb* rt_non_bottom_states;

    /// \brief used during finalizing for the state index in the reduced LTS
    /// \details After partition refinement has finished, the boundary between
    /// bottom and non-bottom states is no longer needed.  Therefore, we use
    /// the same space to store a block number instead.  This block number is
    /// the same as the state number in the reduced LTS.
    state_index te_in_reduced_LTS;

    start_non_bottom_states_or_state_in_reduced_LTS
                          (state_in_block_pointer_lb* s)
      : rt_non_bottom_states(s)
    {}
  } sta;

  /// pointer past the last state in the block
  state_in_block_pointer_lb* end_states;

  /// \brief superblock for BLC sets that this block is part of
  /// \details When a block is split, this superblock is not split, so the
  /// BLC sets do not need to be split.
  BLC_source_type* block_BLC_source;

  /// \brief pointer to refinement data structure
  /// \details When a block needs refinement, information about how to
  /// initialize the states is stored in the refinement data structure.
  /// The pointer mostly points to an element of the list of blocks that need
  /// refinement.
  /// When the block does not need to be refined, this value is nullptr.
  block_that_needs_refinement_type* refinement_info = nullptr;

  /// \brief copy constructor. Required by MSCV.
  block_type_lb(const block_type_lb& other)
    : constellation(other.constellation),
      start_bottom_states(other.start_bottom_states),
      sta(other.sta.rt_non_bottom_states),
      end_states(other.end_states),
      block_BLC_source(other.block_BLC_source),
      refinement_info(other.refinement_info),
      contains_new_bottom_states(other.contains_new_bottom_states),
      is_small_subblock(other.is_small_subblock)
  {}

  /// \brief a boolean that is true iff the block contains new bottom states
  /// \details If a block contains new bottom states, it will be ignored until
  /// `stabilizeB()` handles all blocks with new bottom states.  Such a block
  /// must also be added to the list `m_blocks_with_new_bottom_states`.
  char contains_new_bottom_states = false;

  /// \brief a boolean that is true iff the block is a small subblock
  /// \details If a block has become a small subblock of an earlier split, it
  /// is allowed to go through its states and transitions once.  This is
  /// exploited to avoid the costly handling of NewBotSt.
  ///
  /// Because (almost) every block that is created anew is a small sub-block,
  /// we set the default value to true.
  char is_small_subblock = true;

  /// constructor
  block_type_lb(state_in_block_pointer_lb* start_bottom,
                state_in_block_pointer_lb* start_non_bottom,
                state_in_block_pointer_lb* end,
                constellation_type_lb& new_c,
                BLC_source_type& new_bbs)
    : constellation(&new_c),
      start_bottom_states(start_bottom),
      sta(start_non_bottom),
      end_states(end),
      block_BLC_source(&new_bbs)
  {                                                                             assert(start_bottom<=start_non_bottom);  assert(start_non_bottom<=end);
  }
                                                                                #ifndef NDEBUG
                                                                                  /// \brief print a short block identification for debugging
                                                                                  template<class LTS_TYPE> std::string debug_id_short
                                                                                             (const bisim_partitioner_gj_lazy_BLC<LTS_TYPE>& partitioner) const
                                                                                  { assert(partitioner.m_states_in_blocks.data()<=start_bottom_states);
                                                                                    assert(start_bottom_states<=sta.rt_non_bottom_states);
                                                                                    assert(sta.rt_non_bottom_states<=end_states);
                                                                                    assert(end_states<=partitioner.m_states_in_blocks.data_end());
                                                                                    return "["+std::to_string(std::distance<const state_in_block_pointer_lb*>
                                                                                             (partitioner.m_states_in_blocks.data(), start_bottom_states))+","+
                                                                                        std::to_string(std::distance<const state_in_block_pointer_lb*>
                                                                                                      (partitioner.m_states_in_blocks.data(), end_states))+")";
                                                                                  }

                                                                                  /// \brief print a block identification for debugging
                                                                                  template<class LTS_TYPE> std::string debug_id
                                                                                             (const bisim_partitioner_gj_lazy_BLC<LTS_TYPE>& partitioner) const
                                                                                  {
                                                                                    return (is_small_subblock?"block ":"BLOCK ") + debug_id_short(partitioner);
                                                                                  }
                                                                                #endif
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  mutable check_complexity::block_gj_counter_t work_counter;
                                                                                #endif
};

/// data structure to indicate the sources of super-BLC sets
struct BLC_source_type
{
  /// start of the slice in m_states_in_blocks containing source states
  state_in_block_pointer_lb* start_BLC_source;
  /// end of the slice in m_states_in_blocks containing source states
  state_in_block_pointer_lb* end_BLC_source;
  /// list of BLC sets with transitions starting in these states
  simple_list<BLC_indicators_lb> block_to_constellation;                           static_assert(std::is_trivially_destructible
                                                                                                                        <simple_list<BLC_indicators_lb> >::value);
  BLC_source_type(state_in_block_pointer_lb* new_start,
                  state_in_block_pointer_lb* new_end)
    : start_BLC_source(new_start),
      end_BLC_source(new_end),
      block_to_constellation()
  {                                                                             assert(new_start <= new_end);
  };
                                                                                #ifndef NDEBUG
                                                                                  /// \brief print a BLC-source identification for debugging
                                                                                  template<class LTS_TYPE> std::string debug_id
                                                                                             (const bisim_partitioner_gj_lazy_BLC<LTS_TYPE>& partitioner) const
                                                                                  { assert(partitioner.m_states_in_blocks.data()<=start_BLC_source);
                                                                                    assert(start_BLC_source<end_BLC_source);
                                                                                    assert(end_BLC_source<=partitioner.m_states_in_blocks.data_end());
                                                                                    std::string result("BLC source [");
                                                                                    result += std::to_string(std::distance<const state_in_block_pointer_lb*>
                                                                                                    (partitioner.m_states_in_blocks.data(), start_BLC_source));
                                                                                    result += ",";
                                                                                    result += std::to_string(std::distance<const state_in_block_pointer_lb*>
                                                                                                      (partitioner.m_states_in_blocks.data(), end_BLC_source));
                                                                                    result += ")";
                                                                                    if (start_BLC_source->ref_state->block !=
                                                                                                                   std::prev(end_BLC_source)->ref_state->block)
                                                                                    {
                                                                                      result += " containing";
                                                                                      const state_in_block_pointer_lb* it = start_BLC_source;
                                                                                      do {
                                                                                        result += " ";
                                                                                        result += it->ref_state->block->debug_id(partitioner);
                                                                                        it = it->ref_state->block->end_states;
                                                                                      } while (it < end_BLC_source);
                                                                                    }
                                                                                    return result;
                                                                                  }
                                                                                #endif
};

/// \brief information about a block that needs to be refined
/// \details Because we will refine multiple blocks at a time, we need to store
/// the information about ReachAlw, AvoidSml, AvoidLrg, pot-ReachAlw and
/// HitSmall elsewhere.  These sets are constructed from the super-BLC set for
/// all blocks in the super-BLC source at the same time.
struct block_that_needs_refinement_type
{
  /// \brief distribution of bottom states
  /// \details Bottom states are distributed over the subblocks by placing
  /// them in a specific slice of the bottom states of block `bi`: at the
  /// beginning there will be ReachAlw-bottom states, then AvoidLrg-bottom
  /// states and at the end AvoidSml-bottom states.  The iterators indicate
  /// the place where every slice starts; at the same time, this is the end
  /// of the previous slice.
  ///
  /// contains the beginning of the parts of the bottom states:
  /// `start_bottom_states[ReachAlw] ... start_bottom_states[ReachAlw+1]` contains the states that are guaranteed to be in ReachAlw
  /// `start_bottom_states[AvoidSml] ... start_bottom_states[AvoidSml+1]` contains the states that might remain in AvoidSml
  /// `start_bottom_states[AvoidLrg] ... start_bottom_states[AvoidLrg+1]` contains the states that are guaranteed to be in AvoidLrg

  state_in_block_pointer_lb* start_bottom_states[4];

  /// \brief potential non-bottom states
  /// \details These vectors contain non-bottom states that have been found
  /// when going through predecessors of a subblock.
  std::vector<state_in_block_pointer_lb> potential_non_bottom_states[3];
  std::vector<state_in_block_pointer_lb> potential_non_bottom_states_HitSmall;

  /// \brief large splitter
  /// \details The large splitter is needed because one cannot go through all
  /// its transitions before the coroutines start.  Note that during
  /// `stabilizeB()` it may happen that `large_splitter==nullptr`, namely if
  /// the BLC set is small.
  BLC_indicators_lb* large_splitter;

  /// \brief constructor
  /// \details The constructor initializes AvoidSml to contain all bottom
  /// states (the default).  The block's pointer to `refinement_info` is also
  /// initialized.  Note that there is no destructor that would set
  /// `refinement_info` to nullptr again.
  block_that_needs_refinement_type(block_type_lb& B,
                                   BLC_indicators_lb* a_large_splitter=nullptr)
    : start_bottom_states{B.start_bottom_states, B.start_bottom_states, B.sta.rt_non_bottom_states, B.sta.rt_non_bottom_states},
      potential_non_bottom_states(),
      potential_non_bottom_states_HitSmall(),
      large_splitter(a_large_splitter)
  {                                                                             assert(nullptr==B.refinement_info);
    B.refinement_info = this;
  }

  /// calculate the size of the bottom states that are in subblock coroutine
  state_index bottom_size(enum subblocks coroutine)
  {                                                                             assert(ReachAlw==coroutine ||
                                                                                       AvoidSml==coroutine ||
                                                                                       AvoidLrg==coroutine);
                                                                                assert(start_bottom_states[(coroutine)]<=start_bottom_states[(coroutine)+1]);
    return std::distance(start_bottom_states[coroutine],
                                             start_bottom_states[coroutine+1]);
  }
                                                                                #ifndef NDEBUG
                                                                                  template<class LTS_TYPE>
                                                                                  std::string debug_id(const bisim_partitioner_gj_lazy_BLC<LTS_TYPE>& partitioner) const
                                                                                  {
                                                                                    std::string result("refinement info for ");
                                                                                    result += start_bottom_states[0]->ref_state->block->debug_id(partitioner);
                                                                                    result += ":\n";
                                                                                    const state_in_block_pointer_lb* bott_it = start_bottom_states[0];
                                                                                    assert(bott_it <= start_bottom_states[1]);
                                                                                    if (bott_it < start_bottom_states[1]) {
                                                                                      result += "    ReachAlw = { ";
                                                                                      do {
                                                                                        result += bott_it->ref_state->debug_id_short(partitioner);  result += " ";
                                                                                      } while (++bott_it < start_bottom_states[1]);
                                                                                      result += "}\n";
                                                                                    }
                                                                                    assert(bott_it <= start_bottom_states[2]);
                                                                                    if (bott_it < start_bottom_states[2]) {
                                                                                      result += "    AvoidSml = { ";
                                                                                      do {
                                                                                        result += bott_it->ref_state->debug_id_short(partitioner);  result += " ";
                                                                                      } while (++bott_it < start_bottom_states[2]);
                                                                                      result += "}\n";
                                                                                    }
                                                                                    assert(bott_it <= start_bottom_states[3]);
                                                                                    if (bott_it < start_bottom_states[3]) {
                                                                                      result += "    AvoidLrg = { ";
                                                                                      do {
                                                                                        result += bott_it->ref_state->debug_id_short(partitioner);  result += " ";
                                                                                      } while (++bott_it < start_bottom_states[3]);
                                                                                      result += "}\n";
                                                                                    }
                                                                                    if (!potential_non_bottom_states[0].empty()) {
                                                                                      result += "    pot-ReachAlw = { ";
                                                                                      std::vector<state_in_block_pointer_lb>::const_iterator it = potential_non_bottom_states[0].begin();
                                                                                      do {
                                                                                        result += it->ref_state->debug_id_short(partitioner);  result += " ";
                                                                                      } while (++it != potential_non_bottom_states[0].end());
                                                                                      result += "}\n";
                                                                                    }
                                                                                    if (!potential_non_bottom_states[1].empty()) {
                                                                                      result += "    pot-AvoidSml = { ";
                                                                                      std::vector<state_in_block_pointer_lb>::const_iterator it = potential_non_bottom_states[1].begin();
                                                                                      do {
                                                                                        result += it->ref_state->debug_id_short(partitioner);  result += " ";
                                                                                      } while (++it != potential_non_bottom_states[1].end());
                                                                                      result += "}\n";
                                                                                    }
                                                                                    if (!potential_non_bottom_states[2].empty()) {
                                                                                      result += "    pot-AvoidLrg = { ";
                                                                                      std::vector<state_in_block_pointer_lb>::const_iterator it = potential_non_bottom_states[2].begin();
                                                                                      do {
                                                                                        result += it->ref_state->debug_id_short(partitioner);  result += " ";
                                                                                      } while (++it != potential_non_bottom_states[2].end());
                                                                                      result += "}\n";
                                                                                    }
                                                                                    if (!potential_non_bottom_states_HitSmall.empty()) {
                                                                                      result += "    HitSmall = { ";
                                                                                      std::vector<state_in_block_pointer_lb>::const_iterator it = potential_non_bottom_states_HitSmall.begin();
                                                                                      do {
                                                                                        result += it->ref_state->debug_id_short(partitioner);  result += " ";
                                                                                      } while (++it != potential_non_bottom_states_HitSmall.end());
                                                                                      result += "}\n";
                                                                                    }
                                                                                    result += "    LargeSp = ";
                                                                                    if (nullptr == large_splitter)  {  result += "nullptr";  }
                                                                                    else  {  result += large_splitter->debug_id(partitioner);  }
                                                                                    return result;
                                                                                  }
                                                                                #endif
};

/// information about a constellation
struct constellation_type_lb
{
  /// points to the first state in m_states_in_blocks
  state_in_block_pointer_lb* start_const_states;

  /// points past the last state in m_states_in_blocks
  state_in_block_pointer_lb* end_const_states;

  constellation_type_lb(state_in_block_pointer_lb* const new_start,
                        state_in_block_pointer_lb* const new_end)
    : start_const_states(new_start),
      end_const_states(new_end)
  {}
                                                                                #ifndef NDEBUG
                                                                                  /// \brief print a constellation identification for debugging
                                                                                  template<class LTS_TYPE>
                                                                                  std::string debug_id(const bisim_partitioner_gj_lazy_BLC<LTS_TYPE>& partitioner) const
                                                                                  { assert(partitioner.m_states_in_blocks.data()<=start_const_states);
                                                                                    assert(start_const_states<end_const_states);
                                                                                    assert(end_const_states<=partitioner.m_states_in_blocks.data_end());
                                                                                    return "constellation ["+std::to_string
                                                                                          (std::distance<const state_in_block_pointer_lb*>
                                                                                            (partitioner.m_states_in_blocks.data(), start_const_states))+","+
                                                                                        std::to_string
                                                                                          (std::distance<const state_in_block_pointer_lb*>
                                                                                            (partitioner.m_states_in_blocks.data(), end_const_states))+")";
                                                                                  }
                                                                                #endif
};

} // end namespace bisimulation_gj_lazy_BLC


/*=============================================================================
=                                 main class                                  =
=============================================================================*/


using namespace mcrl2::lts::detail::bisimulation_gj_lazy_BLC;

/// \class bisim_partitioner_gj_lazy_BLC
/// \brief implements the main algorithm for the branching bisimulation quotient
template <class LTS_TYPE>
class bisim_partitioner_gj_lazy_BLC
{
  private:

    using set_of_states_type = std::unordered_set<state_index>;
    using set_of_transitions_type = std::unordered_set<transition_index>;
                                                                                #ifndef NDEBUG
                                                                                  public: // needed for the debugging functions, e.g. debug_id().
                                                                                #endif
    /// \brief automaton that is being reduced
    LTS_TYPE& m_aut;

    // Generic data structures.
    /// \brief information about states
    fixed_vector<state_type_gj_lb> m_states;

    /// \brief transitions ordered per source state
    /// \details This array is used to go through the outgoing transitions of a
    /// state.  The transitions of a given source state are further grouped per
    /// action label, and within every action label per target constellation.
    /// The invisible label (tau) is always the first label.
    fixed_vector<outgoing_transition_type_lb> m_outgoing_transitions;
                                                                  // During refining this contains the index in m_BLC_transition, of which
                                                                  // the transition field contains the index of the transition.
    fixed_vector<transition_type_lb> m_transitions;
    fixed_vector<state_in_block_pointer_lb> m_states_in_blocks;
    state_index no_of_blocks = 1;
    state_index no_of_constellations = 1;
    fixed_vector<transition_index> m_BLC_transitions;
  private:
    std::vector<block_type_lb*> m_blocks_with_new_bottom_states;

    /// The following variable contains all non-trivial constellations.
    std::vector<constellation_type_lb*> m_non_trivial_constellations;

    std::vector<std::pair<BLC_source_type&, simple_list<BLC_indicators_lb>::iterator> >
                                                m_BLC_indicators_to_be_deleted;

    /// \brief true iff branching (not strong) bisimulation has been requested
    const bool m_branching;

    /// \brief true iff divergence-preserving branching bisimulation has been
    /// requested
    /// \details Note that this field must be false if strong bisimulation has
    /// been requested.  There is no such thing as divergence-preserving strong
    /// bisimulation.
    const bool m_preserve_divergence;

    /// The auxiliary function below can be removed, but is now used to express
    /// that the hidden_label_map does not need to be applied, while still
    /// leaving it in the code.
    static typename LTS_TYPE::labels_size_type m_aut_apply_hidden_label_map
                                        (typename LTS_TYPE::labels_size_type l)
    {
      return l; // m_aut.apply_hidden_label_map(l)
    }

    /// The function assumes that m_branching is true and tests whether
    /// transition t is inert during initialisation under that condition
    bool is_inert_during_init_if_branching(const transition& t) const
    {                                                                           assert(m_branching);
      return m_aut.is_tau(m_aut_apply_hidden_label_map(t.label())) &&
             (!m_preserve_divergence || t.from() != t.to());
    }

    /// The function tests whether transition t is inert during initialisation,
    /// i.e. when there is only one source/target block.
    bool is_inert_during_init(const transition& t) const
    {
      return m_branching && is_inert_during_init_if_branching(t);
    }

    /// The function calculates the label index of transition t, where
    /// tau-self-loops get the special index `divergent_label` if
    /// divergence needs to be preserved
    label_index label_or_divergence(const transition& t,
                                    const label_index divergent_label=-2
                                        /* different from null_action */) const
    {
      label_index result = m_aut_apply_hidden_label_map(t.label());             assert(divergent_label!=result);  assert(null_action!=divergent_label);
      if (m_preserve_divergence && (                                            assert(m_branching),
           t.from() == t.to()) &&
          m_aut.is_tau(result))
      {
        return divergent_label;
      }
      return result;
    }
                                                                                #ifndef NDEBUG
                                                                                  /// \brief Checks whether the transition data structure is correct
                                                                                  /// \returns true iff all checks pass
                                                                                  /// \details Checks whether the pointers incoming transitions -> outgoing
                                                                                  /// transitions -> BLC transitions -> incoming transitions are consistent;
                                                                                  /// whether the pointers from states to incoming and outgoing transitions are
                                                                                  /// consistent; whether the pointers from BLC indicators to BLC sets are
                                                                                  /// consistent.
                                                                                  ///
                                                                                  /// If `check_block_to_constellation`, it also checks whether every
                                                                                  /// transition is in one BLC set of its source block.
                                                                                  ///
                                                                                  /// If `check_temporary_complexity_counters`, it also checks that no more
                                                                                  /// work is accounted for in temporary complexity counters.  If
                                                                                  /// `initialisation` holds, all states are treated as non-bottom states (so
                                                                                  /// that later one might handle all bottom states as new bottom states in the
                                                                                  /// very first call to `stabilizeB()`).  In any case, the BLC sets need to be
                                                                                  /// fully initialised.
                                                                                  void check_transitions(const bool initialisation,
                                                                                                         const bool check_temporary_complexity_counters,
                                                                                                         const bool check_block_to_constellation = true) const
                                                                                  {
                                                                                    for(transition_index ti=0; ti<m_transitions.size(); ++ti)
                                                                                    {
                                                                                      const BLC_list_const_iterator btc_ti=
                                                                                               m_transitions[ti].ref_outgoing_transitions->ref_BLC_transitions;
                                                                                      assert(*btc_ti==ti);

                                                                                      const transition& t=m_aut.get_transitions()[ti];
                                                                                      assert(&*m_states[t.to()].start_incoming_transitions<=&t);
                                                                                      if (t.to()+1!=m_aut.num_states())
                                                                                      {
                                                                                        assert(&t<=&*std::prev(m_states[t.to()+1].start_incoming_transitions));
                                                                                      }
                                                                                      else
                                                                                      {
                                                                                        assert(&t<=&m_aut.get_transitions().back());
                                                                                      }

                                                                                      assert(m_states[t.from()].start_outgoing_transitions<=
                                                                                                                   m_transitions[ti].ref_outgoing_transitions);
                                                                                      if (t.from()+1==m_aut.num_states())
                                                                                      {
                                                                                        assert(m_transitions[ti].ref_outgoing_transitions<
                                                                                                                                 m_outgoing_transitions.end());
                                                                                      }
                                                                                      else
                                                                                      {
                                                                                        assert(m_transitions[ti].ref_outgoing_transitions<
                                                                                                            m_states[t.from() + 1].start_outgoing_transitions);
                                                                                      }

                                                                                      assert(m_transitions[ti].
                                                                                               transitions_per_block_to_constellation->start_same_BLC<=btc_ti);
                                                                                      assert(btc_ti<m_transitions[ti].
                                                                                                         transitions_per_block_to_constellation->end_same_BLC);

                                                                                      if (!check_block_to_constellation)
                                                                                      {
                                                                                        continue;
                                                                                      }

                                                                                      const bisimulation_gj_lazy_BLC::block_type_lb& b=*m_states[t.from()].block;
                                                                                      const BLC_source_type& blc_src=*b.block_BLC_source;

                                                                                      const label_index t_label = label_or_divergence(t);
                                                                                      bool found=false;
                                                                                      for(const BLC_indicators_lb& blc: blc_src.block_to_constellation)
                                                                                      {
                                                                                        if (!blc.is_stable())
                                                                                        {
                                                                                          assert(blc.start_same_BLC<=blc.start_marked_BLC);
                                                                                          assert(blc.start_marked_BLC<=blc.end_same_BLC);
                                                                                        }
                                                                                        assert(blc.start_same_BLC<blc.end_same_BLC);
                                                                                        transition& first_t = m_aut.get_transitions()[*blc.start_same_BLC];
                                                                                        assert(&blc_src == m_states[first_t.from()].block->block_BLC_source);
                                                                                        if (t_label == label_or_divergence(first_t) &&
                                                                                            m_states[first_t.to()].block->constellation ==
                                                                                                                         m_states[t.to()].block->constellation)
                                                                                        {
//std::cerr << "Found " << m_transitions[ti].debug_id(*this) << " in " << blc.debug_id(*this) << '\n';
                                                                                          assert(!found);  assert(blc.start_same_BLC <= btc_ti);
                                                                                          assert(btc_ti<blc.end_same_BLC);
                                                                                          assert(&blc == &*m_transitions[ti].transitions_per_block_to_constellation);
                                                                                          found = true;
                                                                                        }
                                                                                      }
                                                                                      assert(found);
                                                                                      if (check_temporary_complexity_counters)
                                                                                      {
                                                                                        block_type_lb& targetb = *m_states[t.to()].block;
                                                                                        const unsigned max_sourceB = check_complexity::log_n-
                                                                                                         check_complexity::ilog2(number_of_states_in_block(b));
                                                                                        const unsigned max_targetC = check_complexity::log_n-
                                                                                              check_complexity::ilog2(number_of_states_in_constellation
                                                                                                                                     (*targetb.constellation));
                                                                                        const unsigned max_targetB = check_complexity::log_n-
                                                                                                   check_complexity::ilog2(number_of_states_in_block(targetb));
                                                                                        mCRL2complexity(&m_transitions[ti],
                                                                                                no_temporary_work(max_sourceB, max_targetC, max_targetB,
                                                                                                !initialisation &&
                                                                                                0==m_states[t.from()].no_of_outgoing_block_inert_transitions),
                                                                                                                                                        *this);
                                                                                      }
                                                                                    }
                                                                                  }

                                                                                  /// \brief Checks whether data structures are consistent
                                                                                  /// \returns true iff all checks pass
                                                                                  /// \details Checks whether states are in their blocks; the pointers outgoing
                                                                                  /// transition (-> BLC transition) -> incoming transition -> outgoing
                                                                                  /// transition are consistent; whether the saC slices (source state, action,
                                                                                  /// target constellation) are correct; whether blocks are correct.
                                                                                  [[nodiscard]]
                                                                                  bool check_data_structures(const std::string& tag, const bool check_temporary_complexity_counters=true) const
                                                                                  {
                                                                                    mCRL2log(log::debug) << "Check data structures: " << tag << ".\n";
                                                                                    assert(m_states.size()==m_aut.num_states());
                                                                                    assert(m_states_in_blocks.size()==m_aut.num_states());
                                                                                    assert(m_transitions.size()==m_aut.num_transitions());
                                                                                    assert(m_outgoing_transitions.size()==m_aut.num_transitions());
                                                                                    assert(m_BLC_transitions.size()==m_aut.num_transitions());

                                                                                    // Check that the elements in m_states are well formed.
                                                                                    for (fixed_vector<state_type_gj_lb>::iterator si=
                                                                                            const_cast<fixed_vector<state_type_gj_lb>&>(m_states).begin();
                                                                                                                                      si<m_states.cend(); si++)
                                                                                    {
                                                                                      const state_type_gj_lb& s=*si;

//if (s.counter != undefined) { std::cerr << "Checking " << s.debug_id(*this) << ": "; }
                                                                                      assert(s.counter==undefined);

                                                                                      // In the following line we need that si is an iterator (not a const_iterator)
                                                                                      assert(std::find(s.block->start_bottom_states, s.block->end_states,
                                                                                                       state_in_block_pointer_lb(si))!=s.block->end_states);

                                                                                      assert(s.ref_states_in_blocks->ref_state==si);

                                                                                      // ensure that in the incoming transitions we first have the transitions
                                                                                      // with label tau, and then the other transitions:
                                                                                      bool maybe_tau=true;
                                                                                      const std::vector<transition>::const_iterator end_it1=
                                                                                            std::next(si)>=m_states.end() ? m_aut.get_transitions().end()
                                                                                                                   : std::next(si)->start_incoming_transitions;
                                                                                      for (std::vector<transition>::const_iterator
                                                                                                            it=s.start_incoming_transitions; it!=end_it1; ++it)
                                                                                      {
                                                                                        const transition& t=*it;
                                                                                        if (m_aut.is_tau(m_aut_apply_hidden_label_map(t.label())))
                                                                                        {
                                                                                          assert(maybe_tau);
                                                                                        }
                                                                                        else
                                                                                        {
                                                                                          maybe_tau=false;
                                                                                        }
                                                                                        // potentially we might test that the transitions are grouped per label
                                                                                      }

                                                                                      // Check that for each state the outgoing transitions satisfy the
                                                                                      // following invariant:  First there are (originally) inert transitions
                                                                                      // (inert transitions may be separated over multiple constellations, so
                                                                                      // we cannot require that the inert transitions come before other
                                                                                      // tau-transitions).  Then there are other transitions sorted per label
                                                                                      // and constellation.
                                                                                      std::unordered_set<std::pair<label_index, const constellation_type_lb*> >
                                                                                                                                           constellations_seen;

                                                                                      maybe_tau=true;
                                                                                      // The construction below is to enable translation on Windows.
                                                                                      const outgoing_transitions_const_it_lb end_it2=
                                                                                            std::next(si)>=m_states.end() ? m_outgoing_transitions.cend()
                                                                                                                   : std::next(si)->start_outgoing_transitions;
                                                                                      for(outgoing_transitions_const_it_lb it=s.start_outgoing_transitions;
                                                                                                                                             it!=end_it2; ++it)
                                                                                      {
                                                                                        const transition& t=m_aut.get_transitions()[*it->ref_BLC_transitions];
                                                                                        assert(m_states.cbegin()+t.from()==si);
                                                                                        assert(m_transitions[*it->ref_BLC_transitions].
                                                                                                                                 ref_outgoing_transitions==it);
                                                                                        if (it->start_same_saC>it) {
                                                                                          assert(it->start_same_saC<m_outgoing_transitions.end());
                                                                                          assert((it+1)->start_same_saC==it->start_same_saC ||
                                                                                                 (it+1)->start_same_saC<=it);
                                                                                        } else {
                                                                                          assert(it+1==m_outgoing_transitions.end() ||
                                                                                                 (it+1)->start_same_saC>it);
                                                                                        }
                                                                                        const label_index t_label = label_or_divergence(t);
                                                                                        // The following for loop is only executed if it is the last transition in the saC-slice.
                                                                                        for(outgoing_transitions_const_it_lb itt=it->start_same_saC;
                                                                                                                 itt<it->start_same_saC->start_same_saC; ++itt)
                                                                                        {
                                                                                          const transition& t1=
                                                                                                            m_aut.get_transitions()[*itt->ref_BLC_transitions];
                                                                                          assert(m_states.cbegin()+t1.from()==si);
                                                                                          assert(label_or_divergence(t1) == t_label);
                                                                                          assert(m_states[t.to()].block->constellation==
                                                                                                                       m_states[t1.to()].block->constellation);
                                                                                        }

                                                                                        const label_index label = label_or_divergence(t);
                                                                                        // Check that if the target constellation, if not new, is equal to the
                                                                                        // target constellation of the previous outgoing transition.
                                                                                        const constellation_type_lb& t_to_constellation=
                                                                                                                        *m_states[t.to()].block->constellation;
                                                                                        if (constellations_seen.count(std::pair(label, &t_to_constellation))>0)
                                                                                        {
                                                                                          assert(it!=s.start_outgoing_transitions);
                                                                                          const transition& old_t=m_aut.get_transitions()
                                                                                                                         [*std::prev(it)->ref_BLC_transitions];
                                                                                          assert(label_or_divergence(old_t)==label);
                                                                                          assert(&t_to_constellation==
                                                                                                                    m_states[old_t.to()].block->constellation);
                                                                                        }
                                                                                        else
                                                                                        {
                                                                                          if (m_branching && m_aut.is_tau(label))
                                                                                          {
                                                                                            assert(maybe_tau);
                                                                                          }
                                                                                          else
                                                                                          {
                                                                                            maybe_tau=false;
                                                                                          }
                                                                                          constellations_seen.emplace(label, &t_to_constellation);
                                                                                        }
                                                                                      }
                                                                                    }
                                                                                    // Check that the elements in m_transitions are well formed.
                                                                                    check_transitions(false, check_temporary_complexity_counters);

                                                                                    // Check that the elements in m_blocks are well formed.
                                                                                    {
                                                                                      set_of_transitions_type all_transitions;
                                                                                      //transition_index actual_no_of_non_constellation_inert_BLC_sets=0;
                                                                                      for (const state_in_block_pointer_lb* si=m_states_in_blocks.data();
                                                                                        m_states_in_blocks.data_end()!=si; si=si->ref_state->block->end_states)
                                                                                      {
                                                                                        const block_type_lb& b=*si->ref_state->block;
                                                                                        const constellation_type_lb& c=*b.constellation;
                                                                                        assert(m_states_in_blocks.data()<=c.start_const_states);
                                                                                        assert(c.start_const_states<=b.start_bottom_states);
                                                                                        assert(b.start_bottom_states<b.sta.rt_non_bottom_states);
                                                                                        assert(b.sta.rt_non_bottom_states<=b.end_states);
                                                                                        assert(b.end_states<=c.end_const_states);
                                                                                        assert(c.end_const_states<=m_states_in_blocks.data_end());
                                                                                        assert(b.block_BLC_source->start_BLC_source<=b.start_bottom_states);
                                                                                        assert(b.end_states<=b.block_BLC_source->end_BLC_source);

                                                                                        unsigned char const max_B=check_complexity::log_n-
                                                                                                         check_complexity::ilog2(number_of_states_in_block(b));
                                                                                        unsigned char const max_C=check_complexity::log_n-check_complexity::
                                                                                                    ilog2(number_of_states_in_constellation(*b.constellation));
                                                                                        for (const state_in_block_pointer_lb*
                                                                                                is=b.start_bottom_states; is!=b.sta.rt_non_bottom_states; ++is)
                                                                                        {
                                                                                          assert(is->ref_state->block==&b);
                                                                                          assert(is->ref_state->no_of_outgoing_block_inert_transitions==0);
                                                                                          if (check_temporary_complexity_counters)
                                                                                          {
                                                                                            mCRL2complexity(is->ref_state,no_temporary_work(max_B,true),*this);
                                                                                          }
                                                                                        }
                                                                                        for (const state_in_block_pointer_lb*
                                                                                                         is=b.sta.rt_non_bottom_states; is!=b.end_states; ++is)
                                                                                        {
                                                                                          assert(is->ref_state->block==&b);
                                                                                          assert(is->ref_state->no_of_outgoing_block_inert_transitions>0);
                                                                                          // Because there cannot be new bottom states among non-bottom states,
                                                                                          // we can always check the temporary work of non-bottom states:
                                                                                          mCRL2complexity(is->ref_state,no_temporary_work(max_B,false),*this);
                                                                                        }
                                                                                        // Because a block has no temporary or new-bottom-state-related
                                                                                        // counters, we can always check its temporary work:
                                                                                        mCRL2complexity(&b, no_temporary_work(max_C, max_B), *this);

                                                                                        const BLC_source_type& blc_src = *b.block_BLC_source;
                                                                                        if (blc_src.start_BLC_source == b.start_bottom_states)
                                                                                        {
                                                                                          assert(blc_src.block_to_constellation.check_linked_list());
                                                                                          for (simple_list<BLC_indicators_lb>::const_iterator
                                                                                                      ind=blc_src.block_to_constellation.begin();
                                                                                                              ind!=blc_src.block_to_constellation.end(); ++ind)
                                                                                          {
                                                                                            assert(ind->start_same_BLC<ind->end_same_BLC);
                                                                                            const transition& first_transition=
                                                                                                               m_aut.get_transitions()[*(ind->start_same_BLC)];
                                                                                            const label_index first_transition_label=
                                                                                                                         label_or_divergence(first_transition);
                                                                                            //if(!is_inert_during_init(first_transition) ||
                                                                                            //   // BLC source and target constellation do not overlap:
                                                                                            //   m_states[first_transition.to()].block->constellation.end_const_states <= blc_src.start_BLC_source ||
                                                                                            //   blc_src.end_BLC_source <= m_states[first_transition.to()].block->constellation.start_const_states)
                                                                                            //{
                                                                                            //  ++actual_no_of_non_constellation_inert_BLC_sets;
                                                                                            //}
                                                                                            for(BLC_list_const_iterator i=ind->start_same_BLC;
                                                                                                                                      i<ind->end_same_BLC; ++i)
                                                                                            {
                                                                                              const transition& t=m_aut.get_transitions()[*i];
                                                                                              assert(m_transitions[*i].transitions_per_block_to_constellation==
                                                                                                                                                          ind);
                                                                                              all_transitions.emplace(*i);
                                                                                              assert(m_states[t.from()].block->block_BLC_source==&blc_src);
                                                                                              assert(m_states[t.to()].block->constellation==
                                                                                                         m_states[first_transition.to()].block->constellation);
                                                                                              assert(label_or_divergence(t)==first_transition_label);
                                                                                              //if (is_inert_during_init(t) &&
                                                                                              //    m_states[t.from()].block->constellation==
                                                                                              //                           m_states[t.to()].block->constellation)
                                                                                              //{
                                                                                              //  // The inert transitions should be in the first element of
                                                                                              //  // `block.to_constellation`:
                                                                                              //  assert(blc_src.block_to_constellation.begin()==ind);
                                                                                              //}
                                                                                            }
                                                                                            if (check_temporary_complexity_counters)
                                                                                            {
                                                                                              mCRL2complexity(ind, no_temporary_work(0 /* counter not used in this algorithm */,
                                                                                                 check_complexity::log_n-check_complexity::ilog2
                                                                                                   (number_of_states_in_constellation(*m_states
                                                                                                       [first_transition.to()].block->constellation))), *this);
                                                                                            }
                                                                                          }
                                                                                        }
                                                                                      }
                                                                                      assert(all_transitions.size()==m_transitions.size());
                                                                                      //assert(actual_no_of_non_constellation_inert_BLC_sets==
                                                                                      //                               no_of_non_constellation_inert_BLC_sets);
                                                                                      // destruct `all_transitions` here
                                                                                    }

                                                                                    // Check that the constellations are well-formed.
                                                                                    const state_in_block_pointer_lb* ci=m_states_in_blocks.data();
                                                                                    assert(m_states_in_blocks.data_end()!=ci);
                                                                                    do
                                                                                    {
                                                                                      const constellation_type_lb& c=*ci->ref_state->block->constellation;
                                                                                      assert(c.start_const_states==ci);
                                                                                      const state_in_block_pointer_lb* bi=ci;
                                                                                      ci=c.end_const_states;
                                                                                      assert(bi<ci);
                                                                                      do
                                                                                      {
                                                                                        const block_type_lb& b=*bi->ref_state->block;
                                                                                        assert(b.start_bottom_states==bi);
                                                                                        assert(b.constellation==&c);
                                                                                        bi=b.end_states;
                                                                                      }
                                                                                      while (bi<ci);
                                                                                    }
                                                                                    while (ci<m_states_in_blocks.data_end());

                                                                                    // Check that the BLC sources are well-formed.
                                                                                    const state_in_block_pointer_lb* bsi=m_states_in_blocks.data();
                                                                                    assert(m_states_in_blocks.data_end()!=bsi);
                                                                                    do
                                                                                    {
                                                                                      const BLC_source_type& bs=*bsi->ref_state->block->block_BLC_source;
                                                                                      assert(bs.start_BLC_source==bsi);
                                                                                      // additional tests on bs... e.g. on its list of BLC sets?
                                                                                      const state_in_block_pointer_lb* bi=bsi;
                                                                                      bsi=bs.end_BLC_source;
                                                                                      assert(bi<bsi);
                                                                                      do
                                                                                      {
                                                                                        const block_type_lb& b=*bi->ref_state->block;
                                                                                        assert(b.start_bottom_states==bi);
                                                                                        assert(b.block_BLC_source==&bs);
                                                                                        bi=b.end_states;
                                                                                      }
                                                                                      while (bi<bsi);
                                                                                    }
                                                                                    while (bsi<m_states_in_blocks.data_end());

                                                                                    // Check that the states in m_states_in_blocks refer to with ref_states_in_block to the right position.
                                                                                    // and that a state is correctly designated as a (non-)bottom state.
                                                                                    for (const state_in_block_pointer_lb*
                                                                                          si=m_states_in_blocks.data(); si<m_states_in_blocks.data_end(); ++si)
                                                                                    {
                                                                                      assert(si==si->ref_state->ref_states_in_blocks);
                                                                                    }

                                                                                    // Check that the blocks in m_blocks_with_new_bottom_states are bottom states.
                                                                                    for(const block_type_lb* bi: m_blocks_with_new_bottom_states)
                                                                                    {
                                                                                      assert(bi->contains_new_bottom_states);
                                                                                    }

                                                                                    // Check that the non-trivial constellations are non trivial.
                                                                                    for(const constellation_type_lb* ci: m_non_trivial_constellations)
                                                                                    {
                                                                                      // There are at least two blocks in a non-trivial constellation.
                                                                                      const block_type_lb& first_bi=*ci->start_const_states->ref_state->block;
                                                                                      const block_type_lb& last_bi=*std::prev(ci->end_const_states)->ref_state->block;
                                                                                      assert(&first_bi != &last_bi);
                                                                                    }
                                                                                    return true;
                                                                                  }

                                                                                  /// \brief Checks the main invariant of the partition refinement algorithm
                                                                                  /// \returns true iff the main invariant holds
                                                                                  /// \details Checks the following invariant:
                                                                                  ///     If a block has a constellation-non-inert transition, then every
                                                                                  ///     bottom state has a constellation-non-inert transition with the same
                                                                                  ///     label to the same target constellation.
                                                                                  /// It is assumed that the BLC data structure is correct, so we conveniently
                                                                                  /// use that to verify the invariant.
                                                                                  ///
                                                                                  /// The function can also check a partial invariant while stabilisation has
                                                                                  /// not yet finished. If calM != nullptr, then we have:
                                                                                  ///     The above invariant may be violated for BLC sets that are still to
                                                                                  ///     be stabilized, as given by the main splitters in calM.
                                                                                  ///     (calM_elt indicates how far stabilization has handled calM already.)
                                                                                  ///     (block_label_to_cotransition indicates the co-splitters that belong
                                                                                  ///     to the main splitters in calM.)
                                                                                  ///     It may also be violated for blocks that contain new bottom states,
                                                                                  ///     as indicated by m_blocks_with_new_bottom_states.
                                                                                  ///
                                                                                  /// Additionally, the function ensures that only transitions in BLC sets
                                                                                  /// satisfying the above conditions are marked:
                                                                                  ///     Transitions may only be marked in BLC sets that are still to be
                                                                                  ///     stabilized, as given by calM (including co-splitters); they may
                                                                                  ///     also be marked if they start in new bottom states, as indicated by
                                                                                  ///     m_blocks_with_new_bottom_states, or if they start in a singleton
                                                                                  ///     block.
                                                                                  [[nodiscard]]
                                                                                  bool check_stability(const std::string& tag,
                                                                                        const std::vector<std::pair<BLC_list_iterator, BLC_list_iterator> >*
                                                                                                                                                calM=nullptr,
                                                                                        const std::pair<BLC_list_iterator,BLC_list_iterator>* calM_elt=nullptr,
                                                                                        const constellation_type_lb* const old_constellation=null_constellation_lb,
                                                                                        const constellation_type_lb* const new_constellation=null_constellation_lb)
                                                                                                                                                          const
                                                                                  {
                                                                                    assert((old_constellation==null_constellation_lb &&
                                                                                            new_constellation==null_constellation_lb   ) ||
                                                                                           (old_constellation!=null_constellation_lb &&
                                                                                            new_constellation!=null_constellation_lb &&
                                                                                            old_constellation!=new_constellation    ));
                                                                                    mCRL2log(log::debug) << "Check stability: " << tag << ".\n";
                                                                                    // visit all BLC sources:
                                                                                    for (const state_in_block_pointer_lb* blc_src_it=m_states_in_blocks.data();
                                                                                     m_states_in_blocks.data_end()!=blc_src_it;
                                                                                     blc_src_it=blc_src_it->ref_state->block->block_BLC_source->end_BLC_source)
                                                                                    {
                                                                                      const BLC_source_type& blc_src=*blc_src_it->ref_state->block->block_BLC_source;
                                                                                      // visit all super-BLC sets in this BLC source:
                                                                                      bool previous_stable=true;
                                                                                      for(simple_list<BLC_indicators_lb>::const_iterator
                                                                                                              ind=blc_src.block_to_constellation.begin();
                                                                                                              ind!=blc_src.block_to_constellation.end(); ++ind)
                                                                                      {
                                                                                        // first check all kinds of possible properties of the super-BLC set
                                                                                        assert(m_BLC_transitions.data()<=ind->start_same_BLC);
                                                                                        assert(ind->start_same_BLC<ind->end_same_BLC);
                                                                                        if (!ind->is_stable())
                                                                                        {
                                                                                          assert(ind->start_same_BLC<=ind->start_marked_BLC);
                                                                                          assert(ind->start_marked_BLC<=ind->end_same_BLC);
                                                                                          previous_stable = false;
                                                                                        }
                                                                                        else
                                                                                        { assert(previous_stable); }
                                                                                        assert(ind->end_same_BLC<=m_BLC_transitions.data_end());
                                                                                        // all transitions in the super-BLC set begin in the same BLC source,
                                                                                        // have the same label and end in the same constellation:
                                                                                        const transition&first_t=m_aut.get_transitions()[*ind->start_same_BLC];
                                                                                        const label_index first_t_label=label_or_divergence(first_t);
                                                                                        const constellation_type_lb&
                                                                                                       to_constln=*m_states[first_t.to()].block->constellation;
                                                                                        for (BLC_list_const_iterator i=ind->start_same_BLC;
                                                                                                                                      i<ind->end_same_BLC; ++i)
                                                                                        {
                                                                                          const transition& t=m_aut.get_transitions()[*i];
                                                                                          assert(&blc_src == m_states[t.from()].block->block_BLC_source);
                                                                                          assert(label_or_divergence(t) == first_t_label);
                                                                                          assert(&to_constln == m_states[t.to()].block->constellation);
                                                                                        }
                                                                                        // now check stability per block:
                                                                                        bool eventual_instability_is_ok = true;
                                                                                        bool all_blocks_are_singletons = true;
                                                                                        for (const state_in_block_pointer_lb* blk_it = blc_src_it;
                                                                                                                 blk_it != blc_src.end_BLC_source;
                                                                                                                 blk_it = blk_it->ref_state->block->end_states)
                                                                                        {
                                                                                          const block_type_lb& b = *blk_it->ref_state->block;
                                                                                          if (1<std::distance(b.start_bottom_states, b.end_states))
                                                                                          { all_blocks_are_singletons = false; }
                                                                                          if (!is_inert_during_init(first_t) || b.constellation != &to_constln)
                                                                                          {
                                                                                            // The transitions from block b in this super-BLC set are
                                                                                            // constellation-inert.  So b should be stable under it.
                                                                                            set_of_states_type all_source_bottom_states;
                                                                                            for (BLC_list_const_iterator i=ind->start_same_BLC;
                                                                                                                                      i<ind->end_same_BLC; ++i)
                                                                                            {
                                                                                              const transition& t = m_aut.get_transitions()[*i];
                                                                                              const state_type_gj_lb& src = m_states[t.from()];
                                                                                              if (&b != src.block)
                                                                                              {
                                                                                                continue;
                                                                                              }
                                                                                              if (src.ref_states_in_blocks < b.sta.rt_non_bottom_states) {
                                                                                                assert(b.start_bottom_states <= src.ref_states_in_blocks);
                                                                                                assert(0 == src.no_of_outgoing_block_inert_transitions);
                                                                                                all_source_bottom_states.emplace(t.from());
                                                                                              } else {
                                                                                                assert(src.ref_states_in_blocks <= b.end_states);
                                                                                                assert(0 != src.no_of_outgoing_block_inert_transitions);
                                                                                              }
                                                                                            }
                                                                                            assert(all_source_bottom_states.size() <= static_cast<std::size_t>
                                                                                                                  (std::distance(b.start_bottom_states,
                                                                                                                                 b.sta.rt_non_bottom_states)));
                                                                                            if (all_source_bottom_states.size() != static_cast<std::size_t>
                                                                                                                 (std::distance(b.start_bottom_states,
                                                                                                                                b.sta.rt_non_bottom_states)) &&
                                                                                                !all_source_bottom_states.empty())
                                                                                            {
                                                                                              // only splitters should be instable.
                                                                                              mCRL2log(log::debug) << "Not all "
                                                                                                  << std::distance(b.start_bottom_states,
                                                                                                                   b.sta.rt_non_bottom_states)
                                                                                                  << (m_branching ? " bottom states in "
                                                                                                                  : " states in ")
                                                                                                  << b.debug_id(*this) << " have a transition in the "
                                                                                                  << ind->debug_id(*this) << ": transitions found from states";
                                                                                              for (const state_index asbc : all_source_bottom_states)
                                                                                              { mCRL2log(log::debug) << ' ' << asbc; }
                                                                                              mCRL2log(log::debug) << '\n';
                                                                                              if (b.contains_new_bottom_states)
                                                                                              {
                                                                                                mCRL2log(log::debug) << "  This is ok because "
                                                                                                          << b.debug_id(*this) << " contains new bottom states.\n";
                                                                                              }
                                                                                              else
                                                                                              { eventual_instability_is_ok = false; }
                                                                                            }
                                                                                          }
                                                                                        }
                                                                                        // now check marked states:
                                                                                        bool eventual_marking_is_ok = true;
                                                                                        if (!ind->is_stable())
                                                                                        {
                                                                                          // only splitters should contain marked transitions.
                                                                                          mCRL2log(log::debug) << ind->debug_id(*this) << " contains "
                                                                                                     << std::distance(ind->start_marked_BLC, ind->end_same_BLC)
                                                                                                                                   << " marked transitions.\n";
                                                                                          eventual_marking_is_ok = false;
                                                                                        }
                                                                                        if (!(eventual_instability_is_ok && eventual_marking_is_ok) && nullptr != calM && calM->begin() != calM->end())
                                                                                        {
                                                                                          std::vector<std::pair<BLC_list_iterator, BLC_list_iterator> >::const_iterator calM_iter = calM->begin();
                                                                                          if (nullptr != calM_elt)
                                                                                          {
                                                                                            for(;;)
                                                                                            {
                                                                                              assert(calM->end() != calM_iter);
                                                                                              if (calM_iter->first <= calM_elt->first && calM_elt->second <= calM_iter->second)
                                                                                              {
                                                                                                break;
                                                                                              }
                                                                                              ++calM_iter;
                                                                                            }
                                                                                            if (calM_elt->first<=ind->start_same_BLC && ind->end_same_BLC<=calM_elt->second)
                                                                                            {
                                                                                              mCRL2log(log::debug) <<"  This is ok because the super-BLC set ("
                                                                                                  << blc_src.debug_id(*this) << " -" << m_aut.action_label(first_t.label())
                                                                                                  << "-> " << to_constln.debug_id(*this)
                                                                                                  << ") is soon going to be a main splitter.\n";
                                                                                              eventual_instability_is_ok = true;
                                                                                              eventual_marking_is_ok = true;
                                                                                            }
                                                                                            else
                                                                                            {
                                                                                              if (old_constellation==&to_constln)
                                                                                              {
                                                                                                const simple_list<BLC_indicators_lb>::const_iterator main_splitter=blc_src.block_to_constellation.next(ind);
                                                                                                if (main_splitter!=blc_src.block_to_constellation.end())
                                                                                                {
                                                                                                  assert(main_splitter->start_same_BLC < main_splitter->end_same_BLC);
                                                                                                  const transition& main_t = m_aut.get_transitions()[*main_splitter->start_same_BLC];
                                                                                                  assert(m_states[main_t.from()].block->block_BLC_source == &blc_src);
                                                                                                  if (first_t_label==label_or_divergence(main_t) &&
                                                                                                      m_states[main_t.to()].block->constellation==
                                                                                                                                             new_constellation)
                                                                                                  {
                                                                                                    if (calM_elt->first<=main_splitter->start_same_BLC && main_splitter->end_same_BLC<=calM_elt->second)
                                                                                                    {
                                                                                                      mCRL2log(log::debug) << "  This is ok because the BLC set (" << blc_src.debug_id(*this) << " -" << m_aut.action_label(first_t.label()) << "-> " << old_constellation->debug_id(*this) << ") is soon going to be a co-splitter.\n";
                                                                                                      eventual_instability_is_ok = true;
                                                                                                      eventual_marking_is_ok = true;
                                                                                                    }
                                                                                                  }
                                                                                                }
                                                                                              }
                                                                                            }
                                                                                            ++calM_iter;
                                                                                          }
                                                                                          for(; !(eventual_instability_is_ok && eventual_marking_is_ok) && calM->end() != calM_iter; ++calM_iter)
                                                                                          {
                                                                                            if (calM_iter->first<=ind->start_same_BLC && ind->end_same_BLC<=calM_iter->second)
                                                                                            {
                                                                                              mCRL2log(log::debug) <<"  This is ok because the BLC set ("
                                                                                                  << blc_src.debug_id(*this) << " -" << m_aut.action_label(first_t.label())
                                                                                                  << "-> " << to_constln.debug_id(*this)
                                                                                                  << ") is going to be a main splitter later.\n";
                                                                                              eventual_instability_is_ok = true;
                                                                                              eventual_marking_is_ok = true;
                                                                                            }
                                                                                            else
                                                                                            {
                                                                                              if (old_constellation == &to_constln)
                                                                                              {
                                                                                                const simple_list<BLC_indicators_lb>::const_iterator main_splitter=blc_src.block_to_constellation.next(ind);
                                                                                                if (main_splitter != blc_src.block_to_constellation.end())
                                                                                                {
                                                                                                  assert(main_splitter->start_same_BLC < main_splitter->end_same_BLC);
                                                                                                  const transition& main_t = m_aut.get_transitions()[*main_splitter->start_same_BLC];
                                                                                                  assert(m_states[main_t.from()].block->block_BLC_source == &blc_src);
                                                                                                  if(first_t_label == label_or_divergence(main_t) &&
                                                                                                     m_states[main_t.to()].block->constellation==
                                                                                                                                             new_constellation)
                                                                                                  {
                                                                                                    if (calM_iter->first<=main_splitter->start_same_BLC && main_splitter->end_same_BLC<=calM_iter->second)
                                                                                                    {
                                                                                                      assert(new_constellation==
                                                                                                                  m_states[main_t.to()].block->constellation);
                                                                                                      mCRL2log(log::debug) << "  This is ok because the BLC "
                                                                                                          "set (" << blc_src.debug_id(*this) << " -"
                                                                                                          << m_aut.action_label(first_t.label())
                                                                                                          << "-> " << old_constellation->debug_id(*this)
                                                                                                          << ") is going to be a co-splitter later.\n";
                                                                                                      eventual_instability_is_ok = true;
                                                                                                      eventual_marking_is_ok = true;
                                                                                                    }
                                                                                                  }
                                                                                                }
                                                                                              }
                                                                                            }
                                                                                          }
                                                                                        }
                                                                                        if (all_blocks_are_singletons)
                                                                                        {
                                                                                          if (!eventual_marking_is_ok)
                                                                                          {
                                                                                            mCRL2log(log::debug) << "  (This is ok because every source block contains only 1 state.)\n";
                                                                                            eventual_marking_is_ok = true;
                                                                                          }
                                                                                        }
                                                                                      }
                                                                                    }
                                                                                    mCRL2log(log::debug) << "Check stability finished: " << tag << ".\n";
                                                                                    return true;
                                                                                  }

                                                                                  /// \brief Prints the list of BLC sets as debug output
                                                                                  void display_BLC_list(const BLC_source_type& blc_src) const
                                                                                  {
                                                                                    mCRL2log(log::debug) << "\n  BLC_List\n";
                                                                                    for(const BLC_indicators_lb& blc_it: blc_src.block_to_constellation)
                                                                                    {
                                                                                      const transition&first_t=m_aut.get_transitions()[*blc_it.start_same_BLC];
                                                                                      const label_index l=label_or_divergence(first_t, (label_index) -2);
                                                                                      mCRL2log(log::debug) << "\n    BLC set "
                                                                                          << std::distance<BLC_list_const_iterator>(m_BLC_transitions.data(),
                                                                                                                               blc_it.start_same_BLC) << " -- "
                                                                                          << std::distance<BLC_list_const_iterator>(m_BLC_transitions.data(),
                                                                                                                                  blc_it.end_same_BLC)
                                                                                          << " of " << ((label_index)-2==l ? "divergent self-loop "
                                                                                                                           : pp(m_aut.action_label(l))+"-")
                                                                                          << "transitions to " << m_states[first_t.to()].block->constellation->debug_id(*this) << ":\n";
                                                                                      for (BLC_list_const_iterator i=blc_it.start_same_BLC; ; ++i)
                                                                                      {
                                                                                        if (i == blc_it.start_marked_BLC)
                                                                                        {
                                                                                          mCRL2log(log::debug) << "        (The BLC set is unstable, and the "
                                                                                                                       " following transitions are marked.)\n";
                                                                                        }
                                                                                        if (i>=blc_it.end_same_BLC)
                                                                                        {
                                                                                          break;
                                                                                        }
                                                                                        const transition& t=m_aut.get_transitions()[*i];
                                                                                        mCRL2log(log::debug) << "        " << t.from() << " -"
                                                                                                           << m_aut.action_label(t.label()) << "-> " << t.to();
                                                                                        if (is_inert_during_init(t) &&
                                                                                            m_states[t.from()].block==m_states[t.to()].block)
                                                                                        {
                                                                                          mCRL2log(log::debug) << " (block-inert)";
                                                                                        }
                                                                                        else if (is_inert_during_init(t) &&
                                                                                                 m_states[t.from()].block->constellation==
                                                                                                                         m_states[t.to()].block->constellation)
                                                                                        {
                                                                                          mCRL2log(log::debug) << " (constellation-inert)";
                                                                                        }
                                                                                        mCRL2log(log::debug) << '\n';
                                                                                      }
                                                                                    }
                                                                                    mCRL2log(log::debug) << "  BLC_List end\n";
                                                                                  }

                                                                                  /// \brief Prints the partition refinement data structure as debug output
                                                                                  void print_data_structures(const std::string& header) const
                                                                                  {
                                                                                    if (!mCRL2logEnabled(log::debug))  {  return;  }
                                                                                    mCRL2log(log::debug) << "========= PRINT DATASTRUCTURE: " << header << " =======================================\n"
                                                                                                            "++++++++++++++++++++    States    ++++++++++++++++++++++++++++\n";
                                                                                    for(state_index si=0; si<m_aut.num_states(); ++si)
                                                                                    {
                                                                                      mCRL2log(log::debug) << "State " << si <<" (" << m_states[si].block->debug_id(*this) << "):\n"
                                                                                                              "  #Inert outgoing transitions: " << m_states[si].no_of_outgoing_block_inert_transitions << "\n"

                                                                                                              "  Incoming transitions:\n";
                                                                                      std::vector<transition>::const_iterator end=(si+1==m_aut.num_states()?m_aut.get_transitions().end():m_states[si+1].start_incoming_transitions);
                                                                                      for(std::vector<transition>::const_iterator it=m_states[si].start_incoming_transitions; it!=end; ++it)
                                                                                      {
                                                                                         mCRL2log(log::debug) << "    " << ptr(*it) << "\n";
                                                                                      }

                                                                                      mCRL2log(log::debug) << "  Outgoing transitions:\n";
                                                                                      label_index t_label=m_aut.tau_label_index();
                                                                                      const constellation_type_lb* to_constln=null_constellation_lb;
                                                                                      for(outgoing_transitions_const_it_lb it=m_states[si].start_outgoing_transitions;
                                                                                                      it!=m_outgoing_transitions.end() &&
                                                                                                      (si+1>=m_aut.num_states() || it!=m_states[si+1].start_outgoing_transitions);
                                                                                                   ++it)
                                                                                      {
                                                                                        const transition& t=m_aut.get_transitions()[*it->ref_BLC_transitions];
                                                                                        bool start_same_saC_valid=
                                                                                            m_outgoing_transitions.cbegin()<=it->start_same_saC &&
                                                                                            it->start_same_saC<m_outgoing_transitions.end();
                                                                                        if (start_same_saC_valid &&
                                                                                            it->start_same_saC->start_same_saC==it &&
                                                                                            it->start_same_saC >= it)
                                                                                        {
                                                                                          // it is at the beginning of a saC slice
                                                                                          const label_index old_t_label=t_label;
                                                                                          t_label=label_or_divergence(t, (label_index) -2);
                                                                                          to_constln=m_states[t.to()].block->constellation;
                                                                                          mCRL2log(log::debug) << "    -  -  -  - saC slice of "
                                                                                                << ((label_index) -2==t_label ? "divergent self-loop "
                                                                                                                        : pp(m_aut.action_label(t_label))+"-")
                                                                                                << "transitions to " << to_constln->debug_id(*this)
                                                                                                << (m_aut.is_tau(t_label) && !m_aut.is_tau(old_t_label)
                                                                                                        ? " -- error: tau-transitions should come first\n"
                                                                                                        : ":\n");
                                                                                        }
                                                                                        mCRL2log(log::debug) << "    " << ptr(t);
                                                                                        if (start_same_saC_valid)
                                                                                        {
                                                                                          if (label_or_divergence(t, (label_index) -2)!=t_label)
                                                                                          {
                                                                                            mCRL2log(log::debug) << " -- error: different label";
                                                                                          }
                                                                                          if (m_states[t.to()].block->constellation!=to_constln)
                                                                                          {
                                                                                            mCRL2log(log::debug) << " -- error: different target " << m_states[t.to()].block->constellation->debug_id(*this);
                                                                                          }
                                                                                          if (it->start_same_saC->start_same_saC == it)
                                                                                          {
                                                                                            // Transition t must be the beginning and/or the end of a saC-slice
                                                                                            if (it->start_same_saC >= it && it > m_outgoing_transitions.cbegin())
                                                                                            {
                                                                                              // Transition t must be the beginning of a saC-slice
                                                                                              const transition& prev_t=m_aut.get_transitions()
                                                                                                                         [*std::prev(it)->ref_BLC_transitions];
                                                                                              if (prev_t.from()==t.from() &&
                                                                                                  label_or_divergence(prev_t)==t_label &&
                                                                                                  m_states[prev_t.to()].block->constellation==
                                                                                                                         m_states[t.to()].block->constellation)
                                                                                              {
                                                                                                mCRL2log(log::debug) << " -- error: not the beginning of a saC-slice";
                                                                                              }
                                                                                            }
                                                                                            if (it->start_same_saC <= it &&
                                                                                                std::next(it) < m_outgoing_transitions.end())
                                                                                            {
                                                                                              // Transition t must be the end of a saC-slice
                                                                                              const transition& next_t=m_aut.get_transitions()
                                                                                                                         [*std::next(it)->ref_BLC_transitions];
                                                                                              if (next_t.from()==t.from() &&
                                                                                                  label_or_divergence(next_t)==t_label &&
                                                                                                  m_states[next_t.to()].block->constellation==
                                                                                                                         m_states[t.to()].block->constellation)
                                                                                              {
                                                                                                mCRL2log(log::debug) << " -- error: not the end of a saC-slice";
                                                                                              }
                                                                                            }
                                                                                          }
                                                                                          else if (it->start_same_saC > it ? it->start_same_saC->start_same_saC > it : it->start_same_saC->start_same_saC < it)
                                                                                          {
                                                                                            mCRL2log(log::debug) << " -- error: not pointing to its own saC-slice";
                                                                                          }
                                                                                        }
                                                                                        mCRL2log(log::debug) << '\n';
                                                                                      }
                                                                                      mCRL2log(log::debug) << "  Ref states in blocks: " << std::distance<fixed_vector<state_type_gj_lb>::const_iterator>(m_states.cbegin(), m_states[si].ref_states_in_blocks->ref_state) << ". Must be " << si <<".\n";
                                                                                      mCRL2log(log::debug) << "---------------------------------------------------\n";
                                                                                    }
                                                                                    mCRL2log(log::debug) << "++++++++++++++++++++ Transitions ++++++++++++++++++++++++++++\n";
                                                                                    for(transition_index ti=0; ti<m_transitions.size(); ++ti)
                                                                                    {
                                                                                      const transition& t=m_aut.get_transitions()[ti];
                                                                                      mCRL2log(log::debug) << "Transition " << ti <<": " << t.from()
                                                                                                                            << " -" << m_aut.action_label(t.label()) << "-> "
                                                                                                                            << t.to() << "\n";
                                                                                    }

                                                                                    mCRL2log(log::debug) << "++++++++++++++++++++ Blocks ++++++++++++++++++++++++++++\n";
                                                                                    for (const state_in_block_pointer_lb* si=m_states_in_blocks.data();
                                                                                        m_states_in_blocks.data_end()!=si; si=si->ref_state->block->end_states)
                                                                                    {
                                                                                      block_type_lb& bi=*si->ref_state->block;
                                                                                      mCRL2log(log::debug) << "  Block " << &bi
                                                                                          << " (" << bi.constellation->debug_id(*this) << ')'
                                                                                          << ":\n  " << std::distance(bi.start_bottom_states,
                                                                                                                                   bi.sta.rt_non_bottom_states)
                                                                                          << (m_branching ? " Bottom state" : " State")
                                                                                          << (1==std::distance(bi.start_bottom_states,
                                                                                                               bi.sta.rt_non_bottom_states) ? ": " : "s: ");
                                                                                      for (const state_in_block_pointer_lb*
                                                                                         sit=bi.start_bottom_states; sit!=bi.sta.rt_non_bottom_states; ++sit)
                                                                                      {
                                                                                        mCRL2log(log::debug) << sit->ref_state->debug_id_short(*this) << "  ";
                                                                                      }
                                                                                      if (m_branching)
                                                                                      {
                                                                                        mCRL2log(log::debug) << "\n  " << std::distance
                                                                                                                 (bi.sta.rt_non_bottom_states, bi.end_states)
                                                                                            << " Non-bottom state" << (1==std::distance
                                                                                                                 (bi.sta.rt_non_bottom_states, bi.end_states)
                                                                                                                  ? ": " : "s: ");
                                                                                        for (const state_in_block_pointer_lb*
                                                                                                    sit=bi.sta.rt_non_bottom_states; sit!=bi.end_states; ++sit)
                                                                                        {
                                                                                          mCRL2log(log::debug) << sit->ref_state->debug_id_short(*this) <<"  ";
                                                                                        }
                                                                                      }
                                                                                      else
                                                                                      {
                                                                                        assert(bi.sta.rt_non_bottom_states==bi.end_states);
                                                                                      }
                                                                                      mCRL2log(log::debug) << "\n";
                                                                                    }

                                                                                    mCRL2log(log::debug) << "++++++++++++++++++++ Constellations ++++++++++++++++++++++++++++\n";
                                                                                    for (const state_in_block_pointer_lb* si=m_states_in_blocks.data();
                                                                                                     m_states_in_blocks.data_end()!=si;
                                                                                                     si=si->ref_state->block->constellation->end_const_states)
                                                                                    {
                                                                                      const constellation_type_lb& ci=*si->ref_state->block->constellation;
                                                                                      mCRL2log(log::debug) << "  " << ci.debug_id(*this) << ":\n";
                                                                                      mCRL2log(log::debug) << "    Blocks in constellation:";
                                                                                      for (const state_in_block_pointer_lb*
                                                                                                            constln_it=ci.start_const_states;
                                                                                                            constln_it<ci.end_const_states; )
                                                                                      {
                                                                                        const block_type_lb& bi=*constln_it->ref_state->block;
                                                                                        mCRL2log(log::debug) << " " << bi.debug_id(*this);
                                                                                        constln_it = bi.end_states;
                                                                                      }
                                                                                      mCRL2log(log::debug) << "\n";
                                                                                    }
                                                                                    mCRL2log(log::debug) << "Non-trivial constellations:";
                                                                                    for (const constellation_type_lb* ci: m_non_trivial_constellations)
                                                                                    {
                                                                                      mCRL2log(log::debug) << " " << ci->debug_id(*this);
                                                                                    }
                                                                                    mCRL2log(log::debug) << "\n++++++++++++++++++++ BLC sources ++++++++++++++++++++++++++++\n";
                                                                                    for (const state_in_block_pointer_lb* si=m_states_in_blocks.data();
                                                                                                     m_states_in_blocks.data_end()!=si;
                                                                                                     si=si->ref_state->block->block_BLC_source->end_BLC_source)
                                                                                    {
                                                                                      const BLC_source_type& blc_src=*si->ref_state->block->block_BLC_source;
                                                                                      mCRL2log(log::debug) << "  " << blc_src.debug_id(*this) << '\n';
                                                                                      display_BLC_list(blc_src);
                                                                                    }

                                                                                    mCRL2log(log::debug) <<
                                                                                         "\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"
                                                                                         "Outgoing transitions:\n";

                                                                                    for (outgoing_transitions_const_it_lb pi = m_outgoing_transitions.cbegin();
                                                                                                                       pi < m_outgoing_transitions.cend(); ++pi)
                                                                                    {
                                                                                      const transition& t=m_aut.get_transitions()[*pi->ref_BLC_transitions];
                                                                                      mCRL2log(log::debug) << "  " << t.from() << " -"
                                                                                                           << m_aut.action_label(t.label()) << "-> " << t.to();
                                                                                      if (m_outgoing_transitions.cbegin()<=pi->start_same_saC &&
                                                                                          pi->start_same_saC<m_outgoing_transitions.end())
                                                                                      {
                                                                                        const transition& t1=m_aut.get_transitions()
                                                                                                                    [*pi->start_same_saC->ref_BLC_transitions];
                                                                                        mCRL2log(log::debug) << "  \t(same saC: " << t1.from() << " -" << m_aut.action_label(t1.label()) << "-> " << t1.to();
                                                                                        const label_index t_label = label_or_divergence(t);
                                                                                        if (pi->start_same_saC->start_same_saC == pi)
                                                                                        {
                                                                                          // Transition t must be the beginning and/or the end of a saC-slice
                                                                                          if (pi->start_same_saC >= pi && pi > m_outgoing_transitions.cbegin())
                                                                                          {
                                                                                            // Transition t must be the beginning of a saC-slice
                                                                                            const transition& prev_t=m_aut.get_transitions()
                                                                                                                         [*std::prev(pi)->ref_BLC_transitions];
                                                                                            if (prev_t.from()==t.from() &&
                                                                                                label_or_divergence(prev_t)==t_label &&
                                                                                                m_states[prev_t.to()].block->constellation==
                                                                                                                        m_states[t.to()].block->constellation)
                                                                                            {
                                                                                              mCRL2log(log::debug) << " -- error: not the beginning of a saC-slice";
                                                                                            }
                                                                                          }
                                                                                          if (pi->start_same_saC <= pi && std::next(pi) < m_outgoing_transitions.end())
                                                                                          {
                                                                                            // Transition t must be the end of a saC-slice
                                                                                            const transition& next_t=m_aut.get_transitions()
                                                                                                                         [*std::next(pi)->ref_BLC_transitions];
                                                                                            if (next_t.from()==t.from() &&
                                                                                                label_or_divergence(next_t)==t_label &&
                                                                                                m_states[next_t.to()].block->constellation==
                                                                                                                         m_states[t.to()].block->constellation)
                                                                                            {
                                                                                              mCRL2log(log::debug) << " -- error: not the end of a saC-slice";
                                                                                            }
                                                                                          }
                                                                                        }
                                                                                        else if (pi->start_same_saC > pi ? pi->start_same_saC->start_same_saC > pi : pi->start_same_saC->start_same_saC < pi)
                                                                                        {
                                                                                          mCRL2log(log::debug) << " -- error: not in its own saC-slice";
                                                                                        }
                                                                                        mCRL2log(log::debug) << ')';
                                                                                      }
                                                                                      mCRL2log(log::debug) << '\n';
                                                                                    }
                                                                                    mCRL2log(log::debug) << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"
                                                                                                            "New bottom blocks to be investigated:";

                                                                                    for(const block_type_lb* bi: m_blocks_with_new_bottom_states)
                                                                                    {
                                                                                      mCRL2log(log::debug) << "  " << bi->debug_id(*this) << '\n';
                                                                                    }

                                                                                    mCRL2log(log::debug) << "\n========= END PRINT DATASTRUCTURE: " << header << " =======================================\n";
                                                                                  }
                                                                                #endif // ifndef NDEBUG
  public:
    /// \brief Calculate the number of equivalence classes
    /// \details The number of equivalence classes (which is valid after the
    /// partition has been constructed) is equal to the number of states in the
    /// bisimulation quotient.
    std::size_t num_eq_classes() const
    {
      return no_of_blocks;
    }


    /// \brief Get the equivalence class of a state
    /// \details After running the minimisation algorithm, this function
    /// produces the number of the equivalence class of a state.  This number
    /// is the same as the number of the state in the minimised LTS to which
    /// the original state is mapped.
    /// \param s state whose equivalence class needs to be found
    /// \returns sequence number of the equivalence class of state s
    state_index get_eq_class(const state_index si) const
    {                                                                           assert(si<m_states.size());
      return m_states[si].block->sta.te_in_reduced_LTS;
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
      // Assign numbers to the blocks (i.e. to the states of the reduced LTS)
      // One could devise a fancy scheme where the block containing state i
      // tries to get block number i; but let's just do something simple now.
      // We no longer need sta.rt_non_bottom_states at this moment, so we can
      // reuse that field to store the block number:
      state_index block_number=0;
      for (state_in_block_pointer_lb*
            si=m_states_in_blocks.data(); m_states_in_blocks.data_end()!=si;
                                           si=si->ref_state->block->end_states)
      {
        block_type_lb& bi=*si->ref_state->block;
        // destruct bi->sta.rt_non_bottom_states; -- trivial
        new (&bi.sta.te_in_reduced_LTS) state_index(block_number);
        ++block_number;
      }

      {
        // For BLC sets with a single block as source, we can directly extract
        // the transitions from it.
        // For BLC sets with multiple blocks as source, we have to go through
        // the transitions to determine which blocks actually have a transition
        // in the BLC set.
        std::remove_reference_t<decltype(m_aut.get_transitions())> T;
        for (state_in_block_pointer_lb*
              si=m_states_in_blocks.data(); m_states_in_blocks.data_end()!=si;)
        {
          const BLC_source_type& blc_src=
                                       *si->ref_state->block->block_BLC_source; //mCRL2complexity(&B, add_work(..., 1), *this);
                                                                                    // Because every block is touched exactly once, we do not store a
                                                                                    // physical counter for this.
          bool single_block = si->ref_state->block ==
                           std::prev(blc_src.end_BLC_source)->ref_state->block;
          for(const BLC_indicators_lb& blc_ind: blc_src.block_to_constellation)
          {                                                                     // mCRL2complexity(&blc_ind, add_work(..., 1), *this);
                                                                                    // Because every BLC set is touched exactly once, we do not store
                                                                                    // a physical counter for this.
                                                                                assert(blc_ind.start_same_BLC<blc_ind.end_same_BLC);
            const transition&
                      first_t=m_aut.get_transitions()[*blc_ind.start_same_BLC];
            const bool is_inert=is_inert_during_init(first_t);
            const state_index new_to=get_eq_class(first_t.to());
            if (single_block)
            {
              // The BLC set has exactly one source block; we just add this
              // transition.
              const state_index new_from=get_eq_class(first_t.from());
              if (!is_inert || new_from!=new_to)
              {
                T.emplace_back(new_from, first_t.label(), new_to);
              }
            }
            else
            {
              // we have to go through the transitions in the super-BLC set
              // to find out which source blocks there are actually.
              std::unordered_set<state_index> new_from_set;
              for (BLC_list_const_iterator it=blc_ind.start_same_BLC;
                                              it != blc_ind.end_same_BLC; ++it)
              {
                const transition& t=m_aut.get_transitions()[*it];               assert(is_inert==is_inert_during_init(t));
                const state_index new_from=get_eq_class(t.from());              assert(new_to==get_eq_class(t.to()));
                if (!is_inert || new_from!=new_to)
                {
                  new_from_set.insert(new_from);
                }
              }
              for (state_index new_from : new_from_set)
              {
                T.emplace_back(new_from, first_t.label(), new_to);
              }
            }
          }
          si = blc_src.end_BLC_source;
        }
        m_aut.get_transitions()=std::move(T);
      }
      //
      // Merge the states, by setting the state labels of each state to the
      // concatenation of the state labels of its equivalence class.

      if (m_aut.has_state_info())   // If there are no state labels
      {                             // this step is not needed
        /* Create a vector for the new labels */
        std::remove_reference_t<decltype(m_aut.state_labels())>
                                                  new_labels(num_eq_classes());

        for(std::size_t i=0; i<m_aut.num_states(); ++i)
        {                                                                       //mCRL2complexity(&m_states[i], add_work(..., 1), *this);
                                                                                    // Because every state is touched exactly once, we do not store a
                                                                                    // physical counter for this.
          const state_index new_index(get_eq_class(i));
          new_labels[new_index]=new_labels[new_index]+m_aut.state_label(i);
        }

        m_aut.set_num_states(num_eq_classes(), false);                          assert(0==m_aut.num_state_labels());
        //m_aut.clear_state_labels();
        m_aut.state_labels()=std::move(new_labels);
      }
      else
      {
        m_aut.set_num_states(num_eq_classes(), false);
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
  private:
                                                                                #ifndef NDEBUG
                                                                                  std::string ptr(const transition& t) const
                                                                                  {
                                                                                    return std::to_string(t.from())+" -"+pp(m_aut.action_label(t.label()))+
                                                                                                                                "-> "+std::to_string(t.to());
                                                                                  }
                                                                                #endif
    /*--------------------------- main algorithm ----------------------------*/

    /*------------- four_way_splitB -- Algorithm 2 of [GJ 2025] -------------*/

    /// \brief return the number of states in block `B`
    state_index number_of_states_in_block(const block_type_lb& B) const
    {                                                                           assert(B.start_bottom_states<B.end_states);
      return std::distance(B.start_bottom_states, B.end_states);
    }
                                                                                #ifndef NDEBUG
                                                                                  /// \brief return the number of states in constellation `C`
                                                                                  state_index number_of_states_in_constellation(const constellation_type_lb& C) const
                                                                                  { assert(C.start_const_states<C.end_const_states);
                                                                                    return std::distance(C.start_const_states, C.end_const_states);
                                                                                  }
                                                                                #endif
    /// \brief swap the contents of `pos1` and `pos2`, assuming they are different
    void swap_states_in_states_in_block_never_equal(
              state_in_block_pointer_lb* pos1, state_in_block_pointer_lb* pos2)
    {                                                                           assert(m_states_in_blocks.data()<=pos1);
      std::swap(*pos1,*pos2);                                                   assert(pos1<m_states_in_blocks.data_end());
      pos1->ref_state->ref_states_in_blocks=pos1;                               assert(m_states_in_blocks.data()<=pos2);
      pos2->ref_state->ref_states_in_blocks=pos2;                               assert(pos2<m_states_in_blocks.data_end());  assert(pos1!=pos2);
    }

    /// \brief swap the contents of `pos1` and `pos2` if they are different
    void swap_states_in_states_in_block(
              state_in_block_pointer_lb* pos1, state_in_block_pointer_lb* pos2)
    {
      if (pos1!=pos2)
      {
        swap_states_in_states_in_block_never_equal(pos1, pos2);
      }
    }
/*
    /// \brief Move the contents of `pos1` to `pos2`, those of `pos2` to `pos3` and those of `pos3` to `pos1`
    /// \details The function requires that `pos3` lies in between `pos1` and
    /// `pos2`.  It also requires that `pos2` and `pos3` are different.
    void swap_states_in_states_in_block_23_never_equal(
              state_in_block_pointer_lb* pos1,
              state_in_block_pointer_lb* pos2,
              state_in_block_pointer_lb* pos3)
    {                                                                           assert(m_states_in_blocks.data()<=pos2);   assert(pos2<pos3);
                                                                                assert(pos1<m_states_in_blocks.data_end());
      if (pos1==pos3)
      {
        std::swap(*pos1,*pos2);
      }
      else
      {                                                                         assert(pos3<pos1);
        const state_in_block_pointer_lb temp=*pos1;
        *pos1=*pos3;
        *pos3=*pos2;
        *pos2=temp;

        pos3->ref_state->ref_states_in_blocks=pos3;
      }
      pos1->ref_state->ref_states_in_blocks=pos1;
      pos2->ref_state->ref_states_in_blocks=pos2;
    }

    /// \brief Move the contents of `pos1` to `pos2`, those of `pos2` to `pos3` and those of `pos3` to `pos1`
    /// \details The function requires that `pos3` lies in between `pos1` and
    /// `pos2`.  The swap is only executed if the positions are different.
    void swap_states_in_states_in_block(
              state_in_block_pointer_lb* pos1,
              state_in_block_pointer_lb* pos2,
              state_in_block_pointer_lb* pos3)
    {
      if (pos2==pos3)
      {
        swap_states_in_states_in_block(pos1,pos2);
      }
      else
      {
        swap_states_in_states_in_block_23_never_equal(pos1,pos2,pos3);
      }
    }
*/
    /// \brief Swap the range [`pos1`, `pos1` + `count`) with the range [`pos2`, `pos2` + `count`)
    /// \details `pos1` must come before `pos2`.
    /// (If the ranges overlap, only swap the non-overlapping part.)
    /// The function requires `count` > 0 and `pos1` < `pos2`
    /// (this is sufficient for how it's used below: to swap new bottom states
    /// into their proper places; also, the work counters assume that
    /// [`assign_work_to`, `assign_work_to` + `count`) is assigned the work.)
    void multiple_swap_states_in_states_in_block(
              state_in_block_pointer_lb* pos1,
              state_in_block_pointer_lb* pos2,
              state_index count
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  , const state_in_block_pointer_lb* assign_work_to,
                                                                                    unsigned char const max_B,
                                                                                    enum check_complexity::counter_type const ctr=check_complexity::
                                                                                                       multiple_swap_states_in_block__swap_state_in_small_block
                                                                                #endif
              )
    {                                                                           assert(count<m_aut.num_states());  assert(m_states_in_blocks.data()<=pos1);
      /* if (pos1 > pos2)  std::swap(pos1, pos2);                            */ assert(pos1<pos2);  assert(pos2<=m_states_in_blocks.data_end()-count);
      {
        std::make_signed_t<state_index> overlap = std::distance(pos2, pos1) + count;
        if (overlap > 0)
        {
          count -= overlap;
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  // If we do not change `assign_work_to`, then there should be no overlap
                                                                                  // between the area starting at `pos2` and the one at `assign_work_to`;
                                                                                  // otherwise it may happen that work is assigned to unexpected counters.
                                                                                  if (pos2==assign_work_to) {
                                                                                    assign_work_to+=overlap;
                                                                                  } else  {  assert(assign_work_to+count<=pos2+overlap ||
                                                                                                    pos2+overlap+count<=assign_work_to);  }
                                                                                #endif
          pos2 += overlap;
        }
      }                                                                         assert(0 < count);
      state_in_block_pointer_lb temp=*pos1;
      while (--count > 0)
      {                                                                         mCRL2complexity(assign_work_to->ref_state, add_work(ctr, max_B), *this);
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  ++assign_work_to;
                                                                                #endif
        *pos1 = *pos2;
        pos1->ref_state->ref_states_in_blocks=pos1;
        ++pos1;
        *pos2 = *pos1;
        pos2->ref_state->ref_states_in_blocks=pos2;
        ++pos2;
      }
      *pos1 = *pos2;
      pos1->ref_state->ref_states_in_blocks=pos1;
      *pos2 = temp;
      pos2->ref_state->ref_states_in_blocks=pos2;
                                                                                #ifndef NDEBUG
                                                                                  for (fixed_vector<state_type_gj_lb>::const_iterator
                                                                                                                si=m_states.cbegin(); si<m_states.cend(); ++si)
                                                                                  {
                                                                                    assert(si==si->ref_states_in_blocks->ref_state);
                                                                                  }
                                                                                #endif
    }

    /// \brief marks the transition indicated by `out_pos`.
    /// \details (We use an outgoing_transitions_it_lb because it points to the
    /// m_BLC_transitions entry that needs to be updated.)
    void mark_BLC_transition(const outgoing_transitions_it_lb out_pos)
    {
      BLC_list_iterator old_pos = out_pos->ref_BLC_transitions;
      BLC_indicators_lb& ind =
               *m_transitions[*old_pos].transitions_per_block_to_constellation; assert(ind.start_same_BLC<=old_pos);
                                                                                assert(old_pos<m_BLC_transitions.data_end());
                                                                                assert(old_pos<ind.end_same_BLC);  assert(!ind.is_stable());
      if (old_pos < ind.start_marked_BLC)
      {
        /* The transition is not marked */                                      assert(ind.start_same_BLC<ind.start_marked_BLC);
        BLC_list_iterator new_pos = std::prev(ind.start_marked_BLC);            assert(ind.start_same_BLC<=new_pos);  assert(new_pos<ind.end_same_BLC);
                                                                                assert(new_pos<m_BLC_transitions.data_end());
        if (old_pos < new_pos)
        {
          std::swap(*old_pos, *new_pos);
          m_transitions[*old_pos].ref_outgoing_transitions->
                                                 ref_BLC_transitions = old_pos; assert(out_pos==m_transitions[*new_pos].ref_outgoing_transitions);
          out_pos->ref_BLC_transitions = new_pos;
        }
        ind.start_marked_BLC = new_pos;
      }

                                                                                #ifndef NDEBUG
                                                                                  for (BLC_list_const_iterator it=m_BLC_transitions.data();
                                                                                                                         it<m_BLC_transitions.data_end(); ++it)
                                                                                  {
                                                                                    assert(m_transitions[*it].ref_outgoing_transitions->ref_BLC_transitions==
                                                                                                                                                           it);
                                                                                    assert(m_transitions[*it].transitions_per_block_to_constellation->
                                                                                                                                           start_same_BLC<=it);
                                                                                    assert(it<
                                                                                      m_transitions[*it].transitions_per_block_to_constellation->end_same_BLC);
                                                                                  }
                                                                                #endif
    }

    /// \brief Move the content of i1 to i2, i2 to i3 and i3 to i1.
    void swap_three_iterators_and_update_m_transitions(
                                                    const BLC_list_iterator i1,
                                                    const BLC_list_iterator i2,
                                                    const BLC_list_iterator i3)
    {                                                                           assert(i3<=i2);  assert(i2<=i1);
      if (i1==i3)
      {
        return;
      }
      if ((i1==i2)||(i2==i3))
      {
        std::swap(*i1,*i3);
        m_transitions[*i1].ref_outgoing_transitions->ref_BLC_transitions = i1;
        m_transitions[*i3].ref_outgoing_transitions->ref_BLC_transitions = i3;
      }
      else  // swap all three elements.
      {
        transition_index temp = *i1;
        *i1=*i2;
        *i2=*i3;
        *i3=temp;
        m_transitions[*i1].ref_outgoing_transitions->ref_BLC_transitions = i1;
        m_transitions[*i2].ref_outgoing_transitions->ref_BLC_transitions = i2;
        m_transitions[*i3].ref_outgoing_transitions->ref_BLC_transitions = i3;
      }
    }

    /// \brief Swap transition `ti` from BLC set `old_BLC_block` to BLC set `new_BLC_block`
    /// \param ti             transition that needs to be swapped
    /// \param new_BLC_block  new BLC set, where the transition should go to
    /// \param old_BLC_block  old BLC set, where the transition was in originally
    /// \returns true iff the last element of `old_BLC_block` has been removed
    /// \details It is assumed that the new BLC set is located precisely before
    /// the old BLC set in `m_BLC_transitions`.
    /// This routine cannot be used in the initialisation phase, but only
    /// during refinement.
    ///
    /// This variant of the swap routine assumes that transition `ti` is only
    /// marked if it is in a singleton block or in a block containing new
    /// bottom states.  In both cases, it is not necessary to maintain
    /// transition markings; so `ti` will always be treated as unmarked, and
    /// the new BLC set must be stable.
    /// (However, it may happen that other transitions in `old_BLC_block` are
    /// marked, and then their marking must be kept.)
    [[nodiscard]]
    bool swap_in_the_doubly_linked_list_LBC_in_blocks_new_constellation(
               const transition_index ti,
               simple_list<BLC_indicators_lb>::iterator new_BLC_block,
               simple_list<BLC_indicators_lb>::iterator old_BLC_block)
    {                                                                           assert(new_BLC_block->is_stable());
      BLC_list_iterator old_position=
               m_transitions[ti].ref_outgoing_transitions->ref_BLC_transitions; assert(old_BLC_block->start_same_BLC <= old_position);
                                                                                assert(old_position<old_BLC_block->end_same_BLC);
                                                                                assert(new_BLC_block->end_same_BLC==old_BLC_block->start_same_BLC);
                                                                                assert(m_transitions[ti].transitions_per_block_to_constellation==old_BLC_block);
                                                                                assert(ti == *old_position);  assert(old_BLC_block->is_stable());
      if (old_position!=old_BLC_block->start_same_BLC)
      {
        std::swap(*old_position,*old_BLC_block->start_same_BLC);
        m_transitions[*old_position].ref_outgoing_transitions->
                                            ref_BLC_transitions = old_position;
        m_transitions[*old_BLC_block->start_same_BLC].
                    ref_outgoing_transitions->ref_BLC_transitions =
                                                 old_BLC_block->start_same_BLC;
      }
      new_BLC_block->end_same_BLC=++old_BLC_block->start_same_BLC;
      m_transitions[ti].transitions_per_block_to_constellation=new_BLC_block;
      return old_BLC_block->start_same_BLC==old_BLC_block->end_same_BLC;
    }

    /// \brief Move transition `t` with transition index `ti` to a new BLC set
    /// \param index_block_B  block forming a new constellation, at the same time target of `t`
    /// \param t              transition that needs to be moved
    /// \param ti             (redundant) transition index of t
    /// \returns true iff a new BLC set for non-constellation-inert transitions has been created
    /// \details Called if the target state of transition `t` switches to a new
    /// constellation; at the moment of calling, the new constellation only
    /// contains block `index_block_B`.
    ///
    /// If the transition is not constellation-inert (or does not remain
    /// constellation-inert), it is moved to a BLC set just after the current
    /// BLC set in its list of BLC sets.  If no suitable BLC set exists yet, it
    /// will be created in that position of the list.  In this way, a main
    /// splitter (i.e. a BLC set with transitions to the new constellation)
    /// will always immediately succeed its co-splitter.
    ///
    /// Counting the number of BLC sets requires that the new block still has
    /// the old constellation number.
    [[nodiscard]]
    bool update_the_doubly_linked_list_LBC_new_constellation(
               block_type_lb& index_block_B,
               const transition& t,
               const transition_index ti)
    {                                                                           assert(m_states[t.to()].block==&index_block_B);
//std::cerr << "update_the_doubly_linked_list_LBC_new_constellation("
//<< index_block_B.debug_id(*this) << ',' << m_transitions[ti].debug_id(*this) << ")\n";
      block_type_lb& from_block=*m_states[t.from()].block;                      assert(&m_aut.get_transitions()[ti] == &t);
      BLC_source_type& blc_src=*from_block.block_BLC_source;
      bool new_block_created = false;                                           assert(blc_src.block_to_constellation.check_linked_list());
      simple_list<BLC_indicators_lb>::iterator this_block_to_constellation=
                      m_transitions[ti].transitions_per_block_to_constellation; assert(this_block_to_constellation->is_stable());
                                                                                #ifndef NDEBUG
                                                                                  // Check whether this_block_to_constellation is in the corresponding list
                                                                                  for (simple_list<BLC_indicators_lb>::const_iterator
                                                                                                i=blc_src.block_to_constellation.begin();
                                                                                                i!=this_block_to_constellation; ++i)
                                                                                  {
      /* The transition will be placed in a BLC set immediately after the BLC*/     assert(i!=blc_src.block_to_constellation.end());
      /* set it came from, so that main splitters (with transitions to the   */   }
      /* new constellation) come after co-splitters (with transitions to the */   assert(this_block_to_constellation!=blc_src.block_to_constellation.end());
      /* old constellation).                                                 */   assert(this_block_to_constellation->start_same_BLC <= m_transitions[ti].ref_outgoing_transitions->ref_BLC_transitions);
                                                                                #endif
      simple_list<BLC_indicators_lb>::iterator next_block_to_constellation=
              blc_src.block_to_constellation.next(this_block_to_constellation);
      const transition* first_t;
      if (next_block_to_constellation==blc_src.block_to_constellation.end() ||
            (first_t=&m_aut.get_transitions()
                           [*(next_block_to_constellation->start_same_BLC)],    assert(m_states[first_t->from()].block->block_BLC_source==&blc_src),
             m_states[first_t->to()].block!=&index_block_B) ||
            label_or_divergence(*first_t)!=label_or_divergence(t))
      {
        // Make a new entry in the list next_block_to_constellation, after the
        // current list element.
        new_block_created = true;
        next_block_to_constellation=blc_src.
              block_to_constellation.emplace_after(this_block_to_constellation,
                             this_block_to_constellation->start_same_BLC,
                             this_block_to_constellation->start_same_BLC,true);
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
        /* The entry will be marked as unstable later                        */   next_block_to_constellation->work_counter=
                                                                                                                     this_block_to_constellation->work_counter;
                                                                                #endif
        //if (!is_inert_during_init(first_t) ||
        //    // the BLC source and exactly one of (old constellation, new constellation) overlap:
        //    (blc_src.end_BLC_source <= index_block_B->start_bottom_states ||
        //     index_block_B->end_states <= blc_src.start_BLC_source) !=
        //    (blc_src.end_BLC_source <= index_block_B->constellation->start_const_states ||
        //     index_block_B->constellation->end_const_states <= blc_src.start_BLC_source))
        //{
        //  ++no_of_non_constellation_inert_BLC_sets;
        //}
      }

      if (swap_in_the_doubly_linked_list_LBC_in_blocks_new_constellation(ti,
                     next_block_to_constellation, this_block_to_constellation))
      {
        blc_src.block_to_constellation.erase(this_block_to_constellation);
        //if (!is_inert_during_init(t) ||
        //    // the old target constellation does not overlap with the BLC source:
        //    blc_src.end_BLC_source <= index_block_B->constellation->start_const_states ||
        //    index_block_B->constellation->end_const_states <= blc_src.start_BLC_source)
        //{
        //  --no_of_non_constellation_inert_BLC_sets;
        //}
      }
                                                                                #ifndef NDEBUG
                                                                                  //check_transitions(false, false);
                                                                                #endif
      return new_block_created;
    }

    /// \brief Swap transition `ti` from BLC set `old_BLC_block` to BLC set `new_BLC_block`
    /// \param ti             transition that needs to be swapped
    /// \param new_BLC_block  new BLC set, where the transition should go to
    /// \param old_BLC_block  old BLC set, where the transition was in originally
    /// \returns true iff the last element of `old_BLC_block` has been removed
    /// \details It is assumed that the new BLC set is located precisely before
    /// the old BLC set in `m_BLC_transitions`.
    /// This routine cannot be used in the initialisation phase, but only
    /// during refinement.
    ///
    /// The stability state of old and new BLC set is always the same.
    /// If the old BLC set is unstable, all new transitions will be marked.
    [[nodiscard]]
    bool swap_in_the_doubly_linked_list_LBC_in_blocks_new_block(
               const transition_index ti,
               simple_list<BLC_indicators_lb>::iterator new_BLC_block,
               simple_list<BLC_indicators_lb>::iterator old_BLC_block,
               const bool mark_all_transitions_in_instable_BLC_sets = false)
    {                                                                           assert(new_BLC_block->end_same_BLC==old_BLC_block->start_same_BLC);
                                                                                assert(new_BLC_block->start_same_BLC<=new_BLC_block->end_same_BLC);
      BLC_list_iterator old_position =
               m_transitions[ti].ref_outgoing_transitions->ref_BLC_transitions; assert(old_BLC_block->start_same_BLC<=old_position);
                                                                                assert(old_position<old_BLC_block->end_same_BLC);  assert(ti==*old_position);
                                                                                assert(m_transitions[ti].transitions_per_block_to_constellation==
                                                                                                                                                old_BLC_block);
      if (old_BLC_block->is_stable() ||
          (mark_all_transitions_in_instable_BLC_sets && (                       assert(new_BLC_block->start_same_BLC==new_BLC_block->start_marked_BLC),
           old_position<old_BLC_block->start_marked_BLC)))
      {
        // The transition is not marked.  Either the two BLC sets are stable
        // (and then the transition should not become marked), or the two BLC
        // sets are unstable (and then the transition should become marked).
        if (old_position!=old_BLC_block->start_same_BLC)
        {
          std::swap(*old_position, *old_BLC_block->start_same_BLC);
          m_transitions[*old_position].ref_outgoing_transitions->
                                              ref_BLC_transitions=old_position;
          m_transitions[*old_BLC_block->start_same_BLC].
                    ref_outgoing_transitions->ref_BLC_transitions=
                                                 old_BLC_block->start_same_BLC;
        }
      }
      else
      {                                                                         assert(!old_BLC_block->is_stable());  assert(!new_BLC_block->is_stable());
        if (!mark_all_transitions_in_instable_BLC_sets &&
            old_position<old_BLC_block->start_marked_BLC)
        {                                                                       assert(old_BLC_block->start_marked_BLC<=old_BLC_block->end_same_BLC);
          // the old state is unmarked, and it should not be marked.
          swap_three_iterators_and_update_m_transitions(old_position,
               old_BLC_block->start_same_BLC, new_BLC_block->start_marked_BLC);
          ++new_BLC_block->start_marked_BLC;
        }
        else
        {                                                                       assert(old_BLC_block->start_same_BLC<=old_BLC_block->start_marked_BLC);
          // the old state is marked. It should remain marked.
          swap_three_iterators_and_update_m_transitions(old_position,
                 old_BLC_block->start_marked_BLC, old_BLC_block->start_same_BLC);
          ++old_BLC_block->start_marked_BLC;
        }
      }
      m_transitions[ti].transitions_per_block_to_constellation=new_BLC_block;
      new_BLC_block->end_same_BLC=++old_BLC_block->start_same_BLC;
      return old_BLC_block->start_same_BLC==old_BLC_block->end_same_BLC;
    }
/*
    /// \brief Update the BLC list of transition `ti`, which now starts in block `new_bi`
    /// \param old_bi             the former block where the source state of `ti` was in
    /// \param new_bi             the current block where the source state of `ti` moves to
    /// \param ti                 index of the transition whose source state moved to a new block
    /// \param old_constellation  target constellation of co-splitters
    /// \details If the transition was part of a stable BLC set, or is
    /// constellation-inert, the new BLC set where it goes to is also stable.
    /// If the transition is part of an unstable BLC set, the order of
    /// main/co-splitters is maintained.  This order states that a co-splitter
    /// (i.e. any BLC set with non-constellation-inert transitions whose target
    /// state is in `old_constellation`) immediately precedes its corresponding
    /// main splitter (i.e. a BLC set with non-constellation-inert transitions
    /// whose target state is in the newest constellation, with the same action
    /// labels as the co-splitter).
    ///
    /// To maintain the order, it may happen that the old BLC set (where `ti`
    /// comes from) needs to be kept even if it becomes empty; then it will be
    /// added to `m_BLC_indicators_to_be_deleted` for deletion after all
    /// transitions of `new_bi` have been handled.
    void update_the_doubly_linked_list_LBC_new_block(
               block_type_lb* const old_bi,
               block_type_lb* const new_bi,
               const transition_index ti,
               constellation_type_lb* old_constellation,
               constellation_type_lb*const new_constellation
                 // used to maintain the order of BLC sets:
                 // main splitter BLC sets (target constellation == new constellation) follow immediately
                 // after co-splitter BLC sets (target constellation == old_constellation) in the BLC sets
               )
    {                                                                           assert(old_bi->block_BLC_source->block_to_constellation.check_linked_list());
      const transition& t=m_aut.get_transitions()[ti];                          assert(new_bi->block_BLC_source->block_to_constellation.check_linked_list());
                                                                                assert(m_states[t.from()].block==new_bi);
      simple_list<BLC_indicators_lb>::iterator this_block_to_constellation=
                      m_transitions[ti].transitions_per_block_to_constellation;
                                                                                #ifndef NDEBUG
                                                                                  // Check whether this_block_to_constellation is in the corresponding list
                                                                                  for (simple_list<BLC_indicators_lb>::const_iterator
                                                                                                    i=old_bi->block_BLC_source->block_to_constellation.begin();
                                                                                                    i!=this_block_to_constellation; ++i)
                                                                                  {
                                                                                    assert(i!=old_bi->block_BLC_source->block_to_constellation.end());
                                                                                  }
                                                                                #endif
      const label_index a=label_or_divergence(t);
      constellation_type_lb* const to_constln=
                                         m_states[t.to()].block->constellation;
      simple_list<BLC_indicators_lb>::iterator new_BLC_block;
      const bool t_is_inert=is_inert_during_init(t);
      if (t_is_inert && to_constln==new_bi->constellation)
      {
          / * Before correcting the BLC lists, we already inserted an empty * / assert(this_block_to_constellation==
          / * BLC_indicator into the list to take the constellation-inert   * /                      old_bi->block_BLC_source->block_to_constellation.begin());
          / * transitions.                                                  * / assert(!new_bi->block_BLC_source->block_to_constellation.empty());
          new_BLC_block=
                      new_bi->block_BLC_source->block_to_constellation.begin(); assert(this_block_to_constellation->start_same_BLC==new_BLC_block->end_same_BLC);
                                                                                #ifndef NDEBUG
                                                                                  if (new_BLC_block->start_same_BLC<new_BLC_block->end_same_BLC) {
                                                                                    const transition& inert_t=m_aut.get_transitions()[*new_BLC_block->start_same_BLC];
                                                                                    assert(new_bi==m_states[inert_t.from()].block);
                                                                                    assert(a==label_or_divergence(inert_t));
                                                                                    assert(to_constln==m_states[inert_t.to()].block->constellation);
                                                                                  }
                                                                                #endif
      }
      else
      {
        transition_index perhaps_new_BLC_block_transition;
        const transition* perhaps_new_BLC_t;
        if (this_block_to_constellation->start_same_BLC!=
                                                    m_BLC_transitions.data() &&
            (perhaps_new_BLC_block_transition=
                    *std::prev(this_block_to_constellation->start_same_BLC),
             perhaps_new_BLC_t=
                 &m_aut.get_transitions()[perhaps_new_BLC_block_transition],
             m_states[perhaps_new_BLC_t->from()].block==new_bi) &&
            a==label_or_divergence(*perhaps_new_BLC_t) &&
            to_constln==m_states[perhaps_new_BLC_t->to()].block->constellation)
        {
          // Found the entry where the transition should go to
          // Move the current transition to the new list.
          new_BLC_block=m_transitions[perhaps_new_BLC_block_transition].
                                        transitions_per_block_to_constellation;
                                                                                #ifndef NDEBUG
                                                                                  if (this_block_to_constellation->is_stable()) { assert(new_BLC_block->is_stable()); }
                                                                                  else { assert(!new_BLC_block->is_stable()); }
                                                                                #endif
        }
        else
        {
          // Make a new entry in the list next_block_to_constellation;

          // We first calculate the position where the new BLC set should go to
          // in new_position.
          // Default position: at the beginning.
          simple_list<BLC_indicators_lb>::iterator
           new_position=new_bi->block_BLC_source->block_to_constellation.end(); assert(!is_inert_during_init(t)||to_constln!=new_bi->constellation);
          if (new_bi->block_BLC_source->block_to_constellation.empty())
          {                                                                     assert(!m_branching);
            / * This is the first transition that is moved.                 * / assert(new_bi->block_BLC_source->block_to_constellation.end()==new_position);
          }
          else
          {
            // default position: place it at the end of the list
            new_position=
                 new_bi->block_BLC_source->block_to_constellation.before_end(); assert(new_bi->block_BLC_source->block_to_constellation.end()!=new_position);
          }
          if (null_constellation_lb!=old_constellation)
          {
            if (t_is_inert &&
                ((to_constln==new_constellation &&
                  new_bi->constellation==old_constellation) ||
                      // < The transition goes from the old constellation to
                      // the splitter block and was constellation-inert before.
                      // It is in a main splitter without (unstable)
                      // co-splitter. We do not need to find the co-splitter.
                 (to_constln==old_constellation &&
                  new_bi->constellation==new_constellation)))
                      // < The formerly constellation-inert transition goes
                      // from the new constellation to the old constellation,
                      // it is in a co-splitter without (unstable) main
                      // splitter, and this co-splitter was handled as the
                      // first splitting action.
            {
              old_constellation=null_constellation_lb;
            }
            else
            {                                                                   assert(old_constellation!=new_constellation);
              // The following comments are all formulated for the case that
              // this_block_to_constellation is a main splitter (except when
              // indicated explicitly).
              simple_list<BLC_indicators_lb>::const_iterator old_co_splitter{};
              constellation_type_lb* co_to_constln;
              if ((old_constellation==to_constln &&
                     // i.e. `this_block_to_constellation` is a co-splitter
                   (old_co_splitter=old_bi->block_BLC_source->
                      block_to_constellation.next(this_block_to_constellation),
                    co_to_constln=new_constellation, true)) ||
                  (new_constellation==to_constln &&
                     // i.e. `this_block_to_constellation` is a main splitter
                   (old_co_splitter=old_bi->block_BLC_source->
                      block_to_constellation.prev(this_block_to_constellation),
                    co_to_constln=old_constellation, true)))
              {
                if (old_bi->block_BLC_source->block_to_constellation.end()!=
                                                               old_co_splitter)
                {
                  // If the co-splitter belonging to
                  // `this_block_to_constellation` exists, then it is
                  // `old_co_splitter` (but if there is no such co-splitter,
                  // `old_co_splitter` could be a different main splitter, a
                  // different co-splitter without main splitter, or a
                  // completely unrelated splitter).

                  // Try to find out whether there is already a corresponding
                  // co-splitter in `new_bi->block.to_constellation`
                  // This co-splitter would be just before `old_co_splitter`
                  // in `m_BLC_transitions`.
                  if (new_bi->block_BLC_source->block_to_constellation.end()!=
                                                                new_position &&
                      m_BLC_transitions.data()<old_co_splitter->start_same_BLC)
                      // i.e. this is not the first transition -- neither the
                      // first to be moved to the new block nor the first in
                      // m_BLC_transitions
                  {
                    // Check the transition in the potential corresponding
                    // new co-splitter:
                    const transition_index perhaps_new_co_spl_transition=
                                   *std::prev(old_co_splitter->start_same_BLC);
                    const transition& perhaps_new_co_spl_t=
                        m_aut.get_transitions()[perhaps_new_co_spl_transition];
                    if(new_bi==m_states[perhaps_new_co_spl_t.from()].block &&
                       a==label_or_divergence(perhaps_new_co_spl_t) &&
                       co_to_constln==m_states
                              [perhaps_new_co_spl_t.to()].block->constellation)
                    {
                      // `perhaps_new_co_spl_transition` is in the
                      // corresponding new co-splitter; place the new BLC set
                      // immediately after this co-splitter in the list
                      // `new_bi->block.to_constellation`.
                      new_position=m_transitions
                                      [perhaps_new_co_spl_transition].
                                        transitions_per_block_to_constellation;
                      if (old_constellation==to_constln)
                      {
                        // (`this_block_to_constellation` was a co-splitter:)
                        // `perhaps_new_co_spl_transition` is in the new main
                        // splitter; place the new BLC set immediately before
                        // this main splitter in the list
                        // `new_bi->block.to_constellation`.
                        new_position=new_bi->block_BLC_source->
                                     block_to_constellation.prev(new_position);
                      }
                                                                                #ifndef NDEBUG
                      / * The new co-splitter was found, and                * /   if (old_co_splitter->start_same_BLC<old_co_splitter->end_same_BLC)
                      / * `old_co_splitter` must have been the old          * /   {
                      / * co-splitter.                                      * /     const transition& co_t=m_aut.get_transitions()
                                                                                                                            [*old_co_splitter->start_same_BLC];
                      / * Now the new main splitter is about to be created. * /     assert(old_bi==m_states[co_t.from()].block ||
                      / * In this case it is ok to delete                   * /            new_bi==m_states[co_t.from()].block);
                      / * `this_block_to_constellation` when it becomes     * /     assert(a==label_or_divergence(co_t));
                      / * empty; therefore we set `old_constellation` in a  * /     assert(co_to_constln==m_states[co_t.to()].block->constellation);
                      / * way that it's going to delete it immediately:     * /   }
                                                                                #endif
                      old_constellation=null_constellation_lb;
                      // We should not use `old_constellation` for anything
                      // else after this point.
                    }
                  }
                }
                else
                {
                  // this_block_to_constellation is a main splitter
                  // but it has no corresponding co-splitter.
                  // If it becomes empty, one can immediately delete it.
                  old_constellation=null_constellation_lb;
                }
              }
              else
              {
                // this_block_to_constellation is neither a main splitter nor
                // a co-splitter.  If it becomes empty, one can immediately
                // delete it.
                old_constellation=null_constellation_lb;
              }
            }
          }
          else if (this_block_to_constellation->is_stable())
          {
            // default position during new bottom splits: at the beginning of
            // the list (but after the BLC set of inert transitions)
            new_position=m_branching
                     ? new_bi->block_BLC_source->block_to_constellation.begin()
                     : new_bi->block_BLC_source->block_to_constellation.end();
          }                                                                     assert(!m_branching ||
                                                                                       new_bi->block_BLC_source->block_to_constellation.end()!=new_position);
          const BLC_list_iterator old_BLC_start=this_block_to_constellation->start_same_BLC;
          new_BLC_block=new_bi->block_BLC_source->block_to_constellation.
                  emplace_after(new_position, old_BLC_start, old_BLC_start,
                                     this_block_to_constellation->is_stable());
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  new_BLC_block->work_counter=this_block_to_constellation->work_counter;
                                                                                #endif
        }
      }
      const bool last_element_removed=
              swap_in_the_doubly_linked_list_LBC_in_blocks_new_block(ti,
                                   new_BLC_block, this_block_to_constellation);

      if (last_element_removed)
      {
        if (null_constellation_lb != old_constellation)
        {
          // Sometimes we could still remove this_block_to_constellation
          // immediately (namely if the new main splitter and the new
          // co-splitter already exist, or if the old co-splitter does not
          // exist at all).  A few such cases are handled above, but other
          // cases would require additional, possibly extensive, checks:
          // if (co_block_found) {
          //   copy more or less the code from above that decides
          //   whether this_block_to_constellation is a main splitter
          //   that has an old co-splitter but not a new co-splitter
          //   or vice versa.
          // }
          m_BLC_indicators_to_be_deleted.push_back
                                                 (this_block_to_constellation);
        }
        else
        {
          // Remove this element.
          old_bi->block_BLC_source->block_to_constellation.
                                            erase(this_block_to_constellation);
        }
      }                                                                         assert(old_bi->block_BLC_source->block_to_constellation.check_linked_list());
                                                                                #ifndef NDEBUG
                                                                                  assert(new_bi->block_BLC_source->block_to_constellation.check_linked_list());
                                                                                  check_transitions(no_of_constellations<=1, false, false);
                                                                                #endif
      return;
    }
*/
    #define SPLIT_LEFT -1 // < left part is already known to be smaller
    #define SPLIT_RIGHT 1 // < right part is already known to be smaller
    #define SPLIT_SMALLER 0 // < need to find out which part is smaller
    /// \brief Splits the super-BLC sets of `BLC_source` at `splitpoint`
    /// \details This procedure splits the super-BLC sets into two, as one step
    /// to creating a single-block BLC source set.  The procedure can be called
    /// in two situations:
    /// - either as part of `four_way_splitB()`, namely in  line 3.19 (right),
    ///   when the NewBotSt coroutine has to go through all transitions in the
    ///   large splitter to find states that cannot be in AvoidLrg.  In this
    ///   case, no transitions are marked, but it is important to keep the
    ///   relationship between a small and a large splitter in other BLC sets,
    ///   so that further calls to `refine_super_BLC()` in line 1.22 can find
    ///   the correct large splitter super-BLC set.
    ///   To ensure this, the parameters `old_constellation` and
    ///   `new_constellation` are included.  Main splitter BLC sets (with
    ///   target constellation==new_constellation) follow immediately after
    ///   co-splitter BLC sets (with target constellation==old_constellation).
    /// - or as part of `stabiliseB()`, namely in lines 5.6 or 5.42, when a
    ///   large subblock with new bottom states has been found.  In this case,
    ///   transitions may be marked, but there is no need to keep the order of
    ///   small / large splitter, as `stabiliseB()` does not stabilise under
    ///   two splitters together.
    ///   In this case, we can simplify the marking of a new BLC set that is
    ///   split off from an unstable BLC set: all transitions can be marked.
    ///   In this case the parameters `old_constellation==nullptr` and
    ///   `new_constellation==nullptr`.
    void make_BLC_simple_split_off_part(BLC_source_type& BLC_source,
                          state_in_block_pointer_lb* const splitpoint,
                          const bool mark_all_transitions_in_instable_BLC_sets,
                          constellation_type_lb* const old_constellation,
                          constellation_type_lb* const new_constellation,
                                          const int split_type = SPLIT_SMALLER)
    {                                                                           assert(BLC_source.start_BLC_source < splitpoint);
//std::cerr << "make_BLC_simple_split_off_part("
//<< BLC_source.debug_id(*this) << ','
//<< std::distance(m_states_in_blocks.data(), splitpoint) << ','
//<< (SPLIT_SMALLER == split_type ? "SPLIT_SMALLER)\n" :
//    (0 > split_type ? (assert(SPLIT_LEFT == split_type),  "SPLIT_LEFT)\n")
//                    : (assert(SPLIT_RIGHT== split_type), "SPLIT_RIGHT)\n")));
      state_in_block_pointer_lb* it;                                            assert(splitpoint < BLC_source.end_BLC_source);
      state_in_block_pointer_lb* end_it;
      if (0 > split_type ||
          (0 >= split_type &&
           std::distance(BLC_source.start_BLC_source, splitpoint) <
                         std::distance(splitpoint, BLC_source.end_BLC_source)))
      {
        /* split off the left part                                           */ assert(SPLIT_LEFT == split_type || SPLIT_SMALLER == split_type);
        it = BLC_source.start_BLC_source;
        end_it = splitpoint;
        BLC_source.start_BLC_source = splitpoint;
        // split_type = SPLIT_LEFT;
      }
      else
      {
        /* split off the right part                                          */ assert(SPLIT_RIGHT == split_type || SPLIT_SMALLER == split_type);
        it = splitpoint;
        end_it = BLC_source.end_BLC_source;
        BLC_source.end_BLC_source = splitpoint;
        // split_type = SPLIT_RIGHT;
      }                                                                         assert(it < end_it);
      BLC_source_type* const new_BLC_source=
         #ifdef USE_POOL_ALLOCATOR
             simple_list<BLC_indicators_lb>::get_pool().
             template construct<BLC_source_type>
         #else
             new BLC_source_type
         #endif
                                (it, end_it);                                   assert(std::distance(it, end_it) <=
      /* loop to visit all blocks in [it...end_it)                           */        std::distance(BLC_source.start_BLC_source,BLC_source.end_BLC_source));
      do
      {
        block_type_lb& current_block = *it->ref_state->block;                   assert(&BLC_source == current_block.block_BLC_source);
        const BLC_indicators_lb* const old_large_splitter =
                nullptr == current_block.refinement_info
                               ? nullptr
                               : current_block.refinement_info->large_splitter;
//std::cerr << "current_block == " << current_block.debug_id(*this);
//if (nullptr != current_block.refinement_info) { std::cerr << " (which is unstable";
//if (nullptr != old_large_splitter)
//{ std::cerr << ", large splitter == " << old_large_splitter->debug_id(*this) << ". Transitions that start in the current block are";
//for (BLC_list_const_iterator it = old_large_splitter->start_same_BLC; it != old_large_splitter->end_same_BLC; ++it)
//{ if (m_states[m_aut.get_transitions()[*it].from()].block == &current_block) { std::cerr << "  " << m_transitions[*it].debug_id_short(*this); } }
//}
//std::cerr << ')'; } std::cerr << '\n';
        current_block.block_BLC_source = new_BLC_source;
        current_block.is_small_subblock = true;
        state_in_block_pointer_lb* const blk_end_it=current_block.end_states;   assert(it < blk_end_it); assert(blk_end_it <= end_it);
        // loop to visit all states in [it...blk_end_it)
        do
        {
          // visit all outgoing transitions of *it and move them to new BLC
          // sets

          // The new BLC sets will be placed immediately before the old BLC
          // sets in m_BLC_transitions.
//std::cerr << "   Now visiting outgoing transitions of " << it->ref_state->debug_id(*this) << '\n';
          outgoing_transitions_it_lb out_it =
                                     it->ref_state->start_outgoing_transitions;
          outgoing_transitions_const_it_lb const
            out_it_end=std::next(it->ref_state)==m_states.end()
                        ? m_outgoing_transitions.end()
                        : std::next(it->ref_state)->start_outgoing_transitions;
          for (; out_it < out_it_end; ++out_it)
          {
            // move transition *out_it from its BLC set to a new BLC set.
            BLC_list_iterator old_position = out_it->ref_BLC_transitions;
            simple_list<BLC_indicators_lb>::iterator
                    old_BLC_set = m_transitions[*old_position].
                                        transitions_per_block_to_constellation; assert(old_BLC_set->start_same_BLC<=old_position);
                                                                                assert(old_position<old_BLC_set->end_same_BLC);
            BLC_list_iterator new_position = old_BLC_set->start_same_BLC;
            simple_list<BLC_indicators_lb>::iterator new_BLC_set;
            const transition& tr = m_aut.get_transitions()[*old_position];
            const label_index a = label_or_divergence(tr);
            const transition* prev_tr;
            if (new_position == m_BLC_transitions.data() ||
                (prev_tr = &m_aut.get_transitions()[*std::prev(new_position)],
                 m_states[prev_tr->from()].ref_states_in_blocks <
                                           new_BLC_source->start_BLC_source) ||
                m_states[prev_tr->from()].ref_states_in_blocks >=
                                              new_BLC_source->end_BLC_source ||
                m_states[tr.to()].block->constellation !=
                                m_states[prev_tr->to()].block->constellation ||
                a != label_or_divergence(*prev_tr))
            {
              // a new BLC set needs to be created (and added to the list of
              // new_BLC_source).

              if (old_BLC_set->is_stable())
              {
                // The old BLC set is stable.  We need to check whether we got
                // a transition to the old or new constellation; in that case,
                // the BLC set is a potential co- or main splitter.
                const constellation_type_lb* to_constln;
                simple_list<BLC_indicators_lb>::const_iterator old_co_splitter;
                constellation_type_lb* co_to_constln;
                transition_index perhaps_new_co_spl_transition;
                const transition* perhaps_new_co_spl_t;
                if (null_constellation_lb != old_constellation &&
                    // we are splitting as part of `four_way_splitB()` and
                    // need to preserve the relationship between a small and a
                    // large splitter (== main and co-splitter)
                    (to_constln = m_states[tr.to()].block->constellation,       assert(nullptr != new_constellation),
                     (to_constln == old_constellation &&
                        // i.e. old_BLC_set is a co-splitter
                        (old_co_splitter =
                           BLC_source.block_to_constellation.next(old_BLC_set),
                         co_to_constln = new_constellation, true)) ||
                     (to_constln == new_constellation &&
                        // i.e. old_BLC_set is a main splitter
                        (old_co_splitter =
                           BLC_source.block_to_constellation.prev(old_BLC_set),
                         co_to_constln = old_constellation, true))) &&
                    BLC_source.block_to_constellation.end()!=old_co_splitter &&
                    // old_BLC_set is a co-splitter or a main splitter, and if
                    // its corresponding main/co-splitter exists, then it is
                    // old_co_splitter (but if there is no such co-splitter,
                    // old_co_splitter could be a different splitter).
                    // It may be that this BLC set will be used in a later call
                    // to refine_super_BLC(), so we need to preserve their
                    // relative positions in the new BLC sets.
                    m_BLC_transitions.data()<old_co_splitter->start_same_BLC &&
                    // if old_co_splitter is the corresponding main/co-
                    // splitter, it likely already has a new corresponding
                    // co-/main splitter.
                    (perhaps_new_co_spl_transition =
                                   *std::prev(old_co_splitter->start_same_BLC),
                     perhaps_new_co_spl_t =
                       &m_aut.get_transitions()[perhaps_new_co_spl_transition],
                     // a transition from the new corresponding splitter
                     m_states[perhaps_new_co_spl_t->from()].block->
                                         block_BLC_source == new_BLC_source) &&
                    a == label_or_divergence(*perhaps_new_co_spl_t) &&
                    co_to_constln == m_states
                             [perhaps_new_co_spl_t->to()].block->constellation)
                {
                  // found that a new main/co-splitter already exists.
                  if (old_constellation==to_constln)
                  {
                    // (`old_BLC_set` was a co-splitter:)
                    // `perhaps_new_co_spl_transition` is in the new main
                    // splitter; place the new BLC set immediately before
                    // this main splitter in the list
                    // `new_BLC_source->block_to_constellation`.
                    new_BLC_set = new_BLC_source->block_to_constellation.
                        emplace(m_transitions[perhaps_new_co_spl_transition].
                                      transitions_per_block_to_constellation,
                                           new_position, new_position, true);
                  }
                  else
                  {
                    // (`old_BLC_set` was a main splitter:)
                    // `perhaps_new_co_spl_transition` is in the new
                    // co-splitter; place the new BLC set immediately
                    // after this co-splitter in the list
                    // `new_BLC_source->block_to_constellation`.
                    new_BLC_set = new_BLC_source->block_to_constellation.
                        emplace_after
                               (m_transitions[perhaps_new_co_spl_transition].
                                      transitions_per_block_to_constellation,
                                           new_position, new_position, true);
                  }
                }
                else
                {
                  // This BLC set does not contain relevant transitions, or
                  // there is no new co-splitter. We need to create the new
                  // super-BLC set and possibly we will have a transition that
                  // moves to the new co-splitter later.

                  // other stable BLC sets go to the beginning of the list
                  new_BLC_set = new_BLC_source->block_to_constellation.
                               emplace_front(new_position, new_position, true);
                }
              }
              else
              {
                // unstable BLC sets go to the end of the list
                new_BLC_set = new_BLC_source->block_to_constellation.
                               emplace_back(new_position, new_position, false);
              }
                                                                                #ifndef NDEBUG
                                                                                  new_BLC_set->work_counter = old_BLC_set->work_counter;
                                                                                #endif
              if (nullptr != current_block.refinement_info &&
                  current_block.refinement_info->large_splitter==&*old_BLC_set)
              {
                current_block.refinement_info->large_splitter = &*new_BLC_set;
              }
            }
            else
            {
              // Already found a suitable BLC set to move the transition to
              new_BLC_set = m_transitions[*std::prev(new_position)].
                                        transitions_per_block_to_constellation;
            }
            // In m_BLC_transitions, the new BLC set should be placed
            // immediately before the old one.
            bool last_element_removed =
                    swap_in_the_doubly_linked_list_LBC_in_blocks_new_block
                                   (*old_position, new_BLC_set, old_BLC_set,
                                    mark_all_transitions_in_instable_BLC_sets);
             // perhaps change the parameter in
            // `swap_in_the_doubly_linked_list_LBC_in_blocks_new_block`
            // to `out_it` or to `old_position`?

            if (last_element_removed)
            {
//const transition& tr=m_aut.get_transitions()[*old_position];
//std::cerr << "The last element has been removed from BLC set (" << BLC_source.debug_id(*this)
//<< " -" << pp(m_aut.action_label(tr.label())) << "-> " << m_states[tr.to()].block->constellation->debug_id(*this) << ").\n";
              if (null_constellation_lb != old_constellation)
              {
                // Sometimes we could still remove this_block_to_constellation
                // immediately (namely if the new main splitter and the new
                // co-splitter already exist, or if the old co-splitter does
                // not exist at all).  A few such cases are handled above, but
                // other cases would require additional, possibly extensive,
                // checks:
                // if (co_block_found) {
                //   copy more or less the code from above that decides
                //   whether this_block_to_constellation is a main splitter
                //   that has an old co-splitter but not a new co-splitter
                //   or vice versa.
                // }

                // We should not remove the old BLC set if it is the large
                // splitter of some other split. (The old BLC set has become
                // empty then actually means: there is no large splitter, all
                // bottom states have a transition in the small splitter, and
                // the split will be trivial.  AvoidSml and ReachAlw will be
                // empty; only AvoidLrg and NewBotSt remain, but because the
                // large splitter is empty, also NewBotSt finishes empty.)
                // It is difficult to check this condition as one would have to
                // go through all blocks in the BLC source to delete it.
//std::cerr << " -- but the empty BLC set retained for later deletion\n";
                m_BLC_indicators_to_be_deleted.emplace_back(BLC_source,
                                                                  old_BLC_set);
              }
              else
              {
                /* remove the old BLC set, as it has become empty            */ assert(old_BLC_set->start_same_BLC==old_BLC_set->end_same_BLC);
                BLC_source.block_to_constellation.erase(old_BLC_set);
              }
            }                                                                   else  {  assert(old_BLC_set->start_same_BLC<old_BLC_set->end_same_BLC);  }
          }
          ++it;
        }
        while (it < blk_end_it);
        if (nullptr != old_large_splitter)
        {                                                                       assert(nullptr != current_block.refinement_info);
          if(old_large_splitter==current_block.refinement_info->large_splitter)
          {
                                                                                #ifndef NDEBUG
                                                                                  for (BLC_list_const_iterator it = old_large_splitter->start_same_BLC;
            /* The large splitter did not really contain any transitions     */                                   it != old_large_splitter->end_same_BLC; ++it)
            /* from current_block                                            */   {
                                                                                    assert(m_states[m_aut.get_transitions()[*it].from()].block!=&current_block);
                                                                                  }
                                                                                #endif
            current_block.refinement_info->large_splitter = nullptr;
//std::cerr << "Set the large_splitter to nullptr\n";
          }
                                                                                #ifndef NDEBUG
                                                                                  else { // ensure that large_splitter is in the list of the new BLC set
//std::cerr << "Now current_block.refinement_info->large_splitter == " << current_block.refinement_info->large_splitter->debug_id(*this) << '\n';
                                                                                    simple_list<BLC_indicators_lb>::const_iterator
                                                                                                       BLC_it = new_BLC_source->block_to_constellation.begin();
                                                                                    do {
                                                                                      assert(new_BLC_source->block_to_constellation.end() != BLC_it);
                                                                                    } while (&*BLC_it++ != current_block.refinement_info->large_splitter);
                                                                                  }
                                                                                #endif
        }
      }
      while (it < end_it);
    }

    /// \brief Splits the super-BLC set of `block_index` so it is a true BLC set
    /// \details Sometimes it is necessary to find exactly the transitions out
    /// of a specific block, with a given label and target constellation; then,
    /// the super-BLC set of this block needs to be split.  The procedure can
    /// be called in two situations:
    /// - either as part of `four_way_splitB()`, namely in  line 3.19 (right),
    ///   when the NewBotSt coroutine has to go through all transitions in the
    ///   large splitter to find states that cannot be in AvoidLrg.  In this
    ///   case, no transitions are marked, but it is important to keep the
    ///   relationship between a small and a large splitter in other BLC sets,
    ///   so that further calls to `refine_super_BLC()` in line 1.22 can find
    ///   the correct large splitter super-BLC set.
    ///   To ensure this, the parameters `old_constellation` and
    ///   `new_constellation` are included.  Main splitter BLC sets (with
    ///   target constellation==new_constellation) follow immediately after
    ///   co-splitter BLC sets (with target constellation==old_constellation).
    /// - or as part of `stabiliseB()`, namely in lines 5.6 or 5.42, when a
    ///   large subblock with new bottom states has been found.  In this case,
    ///   transitions may be marked, but there is no need to keep the order of
    ///   small / large splitter, as `stabiliseB()` does not stabilise under
    ///   two splitters together.
    ///   In this case, we can simplify the marking of a new BLC set that is
    ///   split off from an unstable BLC set: all transitions can be marked.
    ///   In this case the parameters `old_constellation==nullptr` and
    ///   `new_constellation==nullptr`.
    void make_BLC_simple(block_type_lb& block_index,
        const bool mark_all_transitions_in_instable_BLC_sets = false,
        constellation_type_lb* const old_constellation = null_constellation_lb,
        constellation_type_lb* const new_constellation = null_constellation_lb)
    {
//std::cerr << "make_BLC_simple(" << block_index.debug_id(*this)
//<< ',' << mark_all_transitions_in_instable_BLC_sets << ")\n";
      BLC_source_type& BLC_source = *block_index.block_BLC_source;              assert(BLC_source.start_BLC_source <= block_index.start_bottom_states);
                                                                                assert(block_index.end_states <= BLC_source.end_BLC_source);
      state_index half_orig_size = std::distance(BLC_source.start_BLC_source,
                                                  BLC_source.end_BLC_source)/2;
      if (state_index first_part_size = std::distance
                (BLC_source.start_BLC_source, block_index.start_bottom_states);
          0 == first_part_size)
      {
        if (block_index.end_states == BLC_source.end_BLC_source)
        {
          // the BLC block is already simple, nothing needs to be done.
          return;
        }
        make_BLC_simple_split_off_part(BLC_source, block_index.end_states,
                                     mark_all_transitions_in_instable_BLC_sets,
                                         old_constellation, new_constellation);
      } else if (state_index last_part_size = std::distance
                           (block_index.end_states, BLC_source.end_BLC_source);
                 0 == last_part_size)
      {
        make_BLC_simple_split_off_part(BLC_source,
                                     block_index.start_bottom_states,
                                     mark_all_transitions_in_instable_BLC_sets,
                                         old_constellation, new_constellation);
      } else {
        state_in_block_pointer_lb* splitpoint = block_index.end_states;
        // The BLC source needs to be split into three parts.
        if (first_part_size < last_part_size)
        {
          // split off first part from BLC_source
          make_BLC_simple_split_off_part(BLC_source,
                             block_index.start_bottom_states,
                             mark_all_transitions_in_instable_BLC_sets,
                             old_constellation, new_constellation, SPLIT_LEFT);
        }
        else
        {
          // split off last part from BLC_source
          make_BLC_simple_split_off_part(BLC_source,
                            block_index.end_states,
                            mark_all_transitions_in_instable_BLC_sets,
                            old_constellation, new_constellation, SPLIT_RIGHT);
          splitpoint = block_index.start_bottom_states;
        }
        make_BLC_simple_split_off_part(BLC_source, splitpoint,
                                     mark_all_transitions_in_instable_BLC_sets,
                                         old_constellation, new_constellation);
      }

      // Now check if the remaining BLC_source block is small.
      // I assume that this happens not very seldomly when the BLC_source is
      // split into three parts, and it may even happen if it is split into
      // two parts -- namely, if the parts have the same size.
      if (std::distance(BLC_source.start_BLC_source,
                                  BLC_source.end_BLC_source) <= half_orig_size)
      {
        // Yes: then the blocks in it are small subblocks.
        state_in_block_pointer_lb* it = BLC_source.start_BLC_source;
        do
        {
          block_type_lb& current_block = *it->ref_state->block;                 assert(it == current_block.start_bottom_states);
          current_block.is_small_subblock = true;                               assert(it < current_block.end_states);
          it = current_block.end_states;
        }
        while (it < BLC_source.end_BLC_source);                                 assert(it <= BLC_source.end_BLC_source);
      }
    }

    /// \brief reset a range of state counters to `undefined`
    /// \details The function is prepared for a situation when we join the
    /// `block` and `counter` fields together into one `block_plus_counter`.
    /// That is why it checks that only counters of states in block `bi` are
    /// reset.
    void clear_state_counters
             (std::vector<state_in_block_pointer_lb>::const_iterator begin,
              std::vector<state_in_block_pointer_lb>::const_iterator const end,
              block_type_lb& block)
    {
      (void) block; // avoid unused parameter warning (as the parameter is only used in Debug mode)
      while (begin!=end)
      {                                                                         assert(&block==begin->ref_state->block);
        begin->ref_state->counter=undefined;
        ++begin;
      }
    }

    /// \brief Moves the former non-bottom state `si` to the bottom states
    /// \details The block of si is not yet inserted into the set of blocks
    /// with new bottom states.
    void change_non_bottom_state_to_bottom_state
                            (const fixed_vector<state_type_gj_lb>::iterator si)
    {                                                                           assert(m_states.begin()<=si);
      block_type_lb& bi = *si->block;                                           assert(si<m_states.end());
      swap_states_in_states_in_block(si->ref_states_in_blocks,
                                                  bi.sta.rt_non_bottom_states); assert(0 == si->no_of_outgoing_block_inert_transitions);
      bi.sta.rt_non_bottom_states++;                                            assert(!bi.contains_new_bottom_states);
      ++no_of_new_bottom_states;
    }

    /// \brief Makes splitter stable and moves it to the beginning of the list
    void make_stable_and_move_to_start_of_BLC
                      (BLC_source_type& from_blc_src,
                       const simple_list<BLC_indicators_lb>::iterator splitter)
    {                                                                           assert(from_blc_src.block_to_constellation.end()!=splitter);
      splitter->make_stable();                                                  assert(splitter->start_same_BLC<splitter->end_same_BLC);
                                                                                #ifndef NDEBUG
                                                                                  const transition& t=m_aut.get_transitions()[*splitter->start_same_BLC];
                                                                                  assert(&from_blc_src==m_states[t.from()].block->block_BLC_source);
                                                                                #endif
      simple_list<BLC_indicators_lb>& btc=from_blc_src.block_to_constellation;  assert(!btc.empty());
      if (splitter!=btc.begin())
      {
        btc.splice(btc.begin(), btc, splitter);
      }
    }

    /// \brief Move states in a set to a specific position in `m_states_in_block`
    /// \param R       vector of states that need to be moved
    /// \param to_pos  position where the first state in `R` needs to move to
    /// \details The work on this is assigned to the states in vector `R`.
    void move_nonbottom_states_to(const todo_state_vector_lb& R,
                                  state_in_block_pointer_lb* to_pos
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  , state_index new_block_bottom_size
                                                                                #endif
                                                                              )
    {
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  unsigned char const max_B=check_complexity::log_n-
                                                                                                       check_complexity::ilog2(new_block_bottom_size+R.size());
                                                                                #endif
      for (state_in_block_pointer_lb st: R)
      {                                                                         mCRL2complexity(st.ref_state, add_work(check_complexity::
                                                                                               split_block_B_into_R_and_BminR__carry_out_split, max_B), *this);
        swap_states_in_states_in_block(to_pos++,
                                       st.ref_state->ref_states_in_blocks);
      }
      return;
    }
/*
    /// \brief Update all BLC sets after a new block has been created
    /// \param old_bi             index of the old block from which states have been taken
    /// \param new_bi             index of the new block
    /// \param old_constellation  old constellation that was split most recently
    /// \details The old constellation is used to maintain the order of
    /// main/co-splitter pairs in the list of BLC sets (remember that we should
    /// have the main splitter immediately before its co-splitter).
    block_type_lb* update_BLC_sets_new_block(block_type_lb* const old_bi,
                                   block_type_lb* const new_bi,
                                   constellation_type_lb* const old_constellation,
                                   constellation_type_lb* const new_constellation)
    {
      // Algorithm 2, Line 2.42
      // adapt the BLC sets of a new block B in a way that they are consistent
      / * with the previous version...                                      * / assert(!old_bi->block_BLC_source->block_to_constellation.empty());
      if (m_branching)
      {
        BLC_list_iterator start_inert_BLC=old_bi->block_BLC_source->
                                block_to_constellation.begin()->start_same_BLC; // if there are inert transitions, they are here
                                                                                simple_list<BLC_indicators_lb>::iterator new_inert_BLC_set=
        new_bi->block_BLC_source->block_to_constellation.
                         emplace_front(start_inert_BLC, start_inert_BLC, true);
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  assert(start_inert_BLC<
                                                                                       old_bi->block_BLC_source->block_to_constellation.begin()->end_same_BLC);
                                                                                  const transition& perhaps_inert_t=m_aut.get_transitions()[*start_inert_BLC];
                                                                                  assert(m_states[perhaps_inert_t.from()].block==old_bi ||
                                                                                         m_states[perhaps_inert_t.from()].block==new_bi);
                                                                                  if (is_inert_during_init(perhaps_inert_t) &&
                                                                                      m_states[perhaps_inert_t.to()].block->constellation==
                                                                                                                                         old_bi->constellation)
                                                                                  {
                                                                                    // This are really the inert transitions, so we should copy the work
                                                                                    // counter
                                                                                    new_inert_BLC_set->work_counter=
                                                                                        old_bi->block_BLC_source->block_to_constellation.begin()->work_counter;
                                                                                  }
                                                                                #else
                                                                                  (void) new_inert_BLC_set; // avoid unused variable warning
                                                                                #endif
      }

      const state_in_block_pointer_lb* const it_end=new_bi->end_states;
      for (state_in_block_pointer_lb*
                              it=new_bi->start_bottom_states; it_end!=it; ++it)
      {                                                                         assert(new_bi==it->ref_state->block);
        outgoing_transitions_const_it_lb const out_it_end=
                std::next(it->ref_state)==m_states.end()
                        ? m_outgoing_transitions.end()
                        : std::next(it->ref_state)->start_outgoing_transitions;
        for (outgoing_transitions_it_lb out_it=it->ref_state->
                      start_outgoing_transitions; out_it_end!=out_it; ++out_it)
        {
          update_the_doubly_linked_list_LBC_new_block(old_bi, new_bi,
           *out_it->ref_BLC_transitions, old_constellation, new_constellation);
        }
      }

      if (m_branching)
      {                                                                         assert(!new_bi->block_BLC_source->block_to_constellation.empty());
        // If the dummy set inserted before the loop is still empty, we remove
        // it again.
        // Before the loop we inserted an empty BLC set for the inert
        // transitions into new_bi->block.to_constellation.
        // If it is still empty, we have to remove it again.
        simple_list<BLC_indicators_lb>::iterator
            inert_ind=new_bi->block_BLC_source->block_to_constellation.begin();
        if (inert_ind->start_same_BLC==inert_ind->end_same_BLC)
        {                                                                       assert(inert_ind->is_stable());
          new_bi->block_BLC_source->block_to_constellation.erase(inert_ind);
        }
      }

      for (std::vector<simple_list<BLC_indicators_lb>::iterator>::iterator
                                 it=m_BLC_indicators_to_be_deleted.begin();
                                 it<m_BLC_indicators_to_be_deleted.end(); ++it)
      {                                                                         assert((*it)->start_same_BLC==(*it)->end_same_BLC);
                                                                                // the work in this loop can be attributed to the operation that added this BLC
        old_bi->block_BLC_source->block_to_constellation.erase(*it);            // set to m_BLC_indicators_to_be_deleted
      }
      clear(m_BLC_indicators_to_be_deleted);

      // Actually it is not necessary to maintain the order (first stable, then
      // unstable) in the BLC list during the main/co-split phase; this order
      // is only needed in the new bottom split phase.  So it would probably be
      // ok to just leave these co-splitters where they are actually and only
      // make them stable.
      return new_bi;
    }
*/
    /// \brief create a new block and adapt the BLC sets, and reset state counters
    /// \param start_bottom_states      pointer to the first bottom state of the new block in `m_states_in_blocks`
    /// \param start_non_bottom_states  pointer to the first non-bottom state of the new block in `m_states_in_blocks`
    /// \param end_states               pointer past the last state of the new block in `m_states_in_blocks`
    block_type_lb* create_new_block(
                      state_in_block_pointer_lb* start_bottom_states,
                      state_in_block_pointer_lb* const start_non_bottom_states,
                      state_in_block_pointer_lb* const end_states,
                      block_type_lb& old_block_index)
    {
      // Algorithm 2, Line 2.41
      constellation_type_lb& constellation=*old_block_index.constellation;      assert(constellation.start_const_states<=start_bottom_states);
                                                                                assert(old_block_index.block_BLC_source->start_BLC_source<=start_bottom_states);
                                                                                assert(start_bottom_states<end_states);
      block_type_lb& new_block_index = *
                #ifdef USE_POOL_ALLOCATOR
                    simple_list<BLC_indicators_lb>::get_pool().
                    template construct<block_type_lb>
                #else
                    new block_type_lb
                #endif
                     (start_bottom_states, start_non_bottom_states, end_states,
                             constellation, *old_block_index.block_BLC_source); assert(end_states<=constellation.end_const_states);
      ++no_of_blocks;                                                           assert(end_states<=old_block_index.block_BLC_source->end_BLC_source);
                                                                                #ifndef NDEBUG
                                                                                  new_block_index.work_counter=old_block_index.work_counter;
                                                                                #endif
      for(; start_bottom_states<start_non_bottom_states; ++start_bottom_states)
      {                                                                         assert(0==
                                                                                       start_bottom_states->ref_state->no_of_outgoing_block_inert_transitions);
                                                                                assert(&old_block_index==start_bottom_states->ref_state->block);
        start_bottom_states->ref_state->block=&new_block_index;                 assert(start_bottom_states->ref_state->counter==undefined);
      }
      for (; start_bottom_states<end_states; ++start_bottom_states)
      {                                                                         assert(&old_block_index==start_bottom_states->ref_state->block);
        start_bottom_states->ref_state->block=&new_block_index;                 assert(0!=
                                                                                       start_bottom_states->ref_state->no_of_outgoing_block_inert_transitions);
        start_bottom_states->ref_state->counter=undefined;
      }

      return &new_block_index;
      // Algorithm 2, Line 2.42
      /*return update_BLC_sets_new_block(&old_block_index, new_block_index,
                                         old_constellation, new_constellation);*/
    }

    /// \brief makes incoming transitions from block `NewBotSt_block_index` non-block-inert
    void check_incoming_tau_transitions_become_noninert(
          block_type_lb& NewBotSt_block_index,
          state_in_block_pointer_lb* start_bottom,
          state_in_block_pointer_lb* const end_non_bottom)
    {
      for (; start_bottom!=end_non_bottom; ++start_bottom)
      {
        std::vector<transition>::const_iterator const in_it_end=
           std::next(start_bottom->ref_state)>=m_states.end()
              ? m_aut.get_transitions().end()
              : std::next(start_bottom->ref_state)->start_incoming_transitions; assert(start_bottom->ref_state->block!=&NewBotSt_block_index);
        for (std::vector<transition>::iterator
                 in_it=start_bottom->ref_state->start_incoming_transitions;
                    in_it!=in_it_end &&
                    m_aut.is_tau(m_aut_apply_hidden_label_map(in_it->label()));
                                                                       ++in_it)
        {
          const fixed_vector<state_type_gj_lb>::iterator
                                           from=m_states.begin()+in_it->from(); assert(m_states[in_it->to()].ref_states_in_blocks==start_bottom);
          if (&NewBotSt_block_index==from->block)
          {
            // make_transition_non_block_inert(*in_it);
            if (0== --from->no_of_outgoing_block_inert_transitions)
            {
              change_non_bottom_state_to_bottom_state(from);
            }
          }
        }
      }
    }

    /// \brief find the next constellation after `splitter_it`'s in the `same_saC` slice of the outgoing transitions
    /// \details Assumes that the BLC sets are fully initialized.
    BLC_indicators_lb* next_target_constln_in_same_saC(
                               state_in_block_pointer_lb const src,
                               BLC_list_const_iterator const splitter_it) const
    {                                                                           assert(m_states.begin()+m_aut.get_transitions()[*splitter_it].from()==
                                                                                                                                                src.ref_state);
      outgoing_transitions_const_it_lb
                   out_it=m_transitions[*splitter_it].ref_outgoing_transitions;
      if (out_it<out_it->start_same_saC)
      {
        out_it=out_it->start_same_saC;
      }
      ++out_it;
      outgoing_transitions_const_it_lb const
                out_it_end=std::next(src.ref_state)>=m_states.end()
                        ? m_outgoing_transitions.end()
                        : std::next(src.ref_state)->start_outgoing_transitions;
      if (out_it<out_it_end)
      {
        return &*m_transitions[*out_it->ref_BLC_transitions].
                                        transitions_per_block_to_constellation;
      }
      else
      {
        return nullptr;
      }
    }

    /// \brief split a block (using main and co-splitter) into up to four subblocks
    /// \details `small_splitter` contains transitions to the newest
    /// constellation `new_constellation`; this serves to stabilize block `bi`
    /// after constellation `new_constellation` was split off from
    /// `old_constellation`.  Because the new constellation is known to be
    /// *small*, the main splitter can be read completely.
    ///
    /// If `large_splitter != nullptr`, this is a true four-way-split, and
    /// `large_splitter` consists of transitions with the same label as
    /// `small_splitter` but with target constellation `old_constellation`.
    /// It is unknown whether `large_splitter` is small or large, but we are
    /// able to quickly determine, for states that have a transition in
    /// `small_splitter`, whether they have one in the co-splitter as well
    /// (using the `start_same_saC` pointers).
    ///
    /// If `large_splitter == nullptr`, then `small_splitter` contains
    /// transitions that have just become non-constellation-inert by splitting
    /// the constellation (and the co-splitter would contain transitions that
    /// are still constellation-inert).
    ///
    /// The function refines `bi` into up to four subblocks consisting of the
    /// following states:
    /// - **ReachAlw:** states that can reach always all splitters provided,
    ///   and their block-inert predecessors
    /// - **AvoidSml:** states that cannot inertly reach `small_splitter`,
    ///   although `small_splitter!=nullptr`, and their block-inert
    ///   predecessors
    /// - **AvoidLrg:** states that cannot inertly reach `large_splitter`,
    ///   although `large_splitter!=nullptr`, and their block-inert
    ///   predecessors
    /// - **NewBotSt:** states that can block-inertly reach multiple of the
    ///   above subsets.  This will include new bottom states and will later
    ///   need to be stabilized under all outgoing BLC sets.
    ///
    /// The bottom states can always be distributed over the first three
    /// subsets; after that, one has to find block-inert predecessors for each
    /// subset to extend it.  NewBotSt mostly starts out empty, but sometimes a
    /// search in one of the other subsets adds a state to it.
    ///
    /// To ensure that the search through block-inert predecessors is quick, it
    /// is broken off after three subblocks have been completed; all remaining
    /// states then must be in the unfinished subblock.  In this way, every
    /// action during the search for block-inert predecessors can be assigned
    /// to a _small_ subblock: either to a state in it, or an incoming or an
    /// outgoing transition.
    ///
    /// \param bri                information about the block being split
    /// \param small_splitter     small BLC set under which the block needs to be stabilized
    /// \returns block index of the ReachAlw subblock if it exists; or `null_block_lb` if ReachAlw is empty
    block_type_lb* four_way_splitB(block_that_needs_refinement_type& bri,
        constellation_type_lb* const old_constellation = null_constellation_lb,
        constellation_type_lb* const new_constellation = null_constellation_lb)
    {
//std::cerr << "four_way_splitB(" << bri.debug_id(*this) << ")\n";
      block_type_lb& bi=*bri.start_bottom_states[0]->ref_state->block;          assert(1<number_of_states_in_block(bi));
                                                                                assert(!bi.contains_new_bottom_states);
      /// \brief proven non-bottom states
      /// \details These vectors contain all non-bottom states of which the
      /// procedure has proven that they are in the respective subblock, unless
      /// the corresponding coroutine has been aborted; all their block-inert
      /// successors are already in the subblock.
      ///
      /// The variable is declared `static` to avoid repeated deallocations and
      /// reallocations while the algorithm runs many refinements.
      ///
      /// The fourth entry in this array is for NewBotSt; it should be in the
      /// same array to allow to find the three other arrays with coroutine^1,
      /// coroutine^2 and coroutine^3.
      static todo_state_vector_lb non_bottom_states[4];

      #define non_bottom_states_NewBotSt non_bottom_states[3]

      // Non-bottom states have a `counter` field that indicates their subblock
      // status: the field contains the sum of a base value, that indicates
      // which subblock they are (potentially) in, and a counter that indicates
      // how many block-inert successors still neeed to be checked.

      #define bottom_and_non_bottom_size(coroutine) (                           assert(aborted!=status[(coroutine)]), \
            bri.bottom_size((coroutine))+non_bottom_states[(coroutine)].size())

      /// \brief next unhandled co-splitter transition
      /// \details NewBotSt may go through the co-splitter transitions at some
      /// point of the algorithm; this iterator is used to store which
      /// transition NewBotSt will handle next.  (The variable is already
      /// declared here just for initialisation.)
      BLC_list_iterator large_splitter_iter_NewBotSt;
      BLC_list_const_iterator large_splitter_iter_end_NewBotSt;
      bool large_splitter_is_known_to_be_a_strict_BLC_set = false;

      if (nullptr != bri.large_splitter /* needed for correctness */)
      {
        // This is a normal main/co-split (where `small_splitter` contains
        // transitions to the _small_ new constellation and `large_splitter`
        // transitions from the same block with the same label to the old
        // constellation).  None of these transitions are constellation-inert.

        // It may happen that large_splitter is actually a super-BLC set.

        large_splitter_iter_NewBotSt = bri.large_splitter->start_same_BLC;
        large_splitter_iter_end_NewBotSt = bri.large_splitter->end_same_BLC;
      }
      else
      {
        // This is a tau main split of the old constellation (where
        // `small_splitter` contains tau-transitions from the old constellation
        // to the _small_ new constellation), or a tau co-split of the new
        // constellation (where `small_splitter` contains tau-transitions from
        // the new constellation to the old constellation).
        // The other splitter is missing because these transitions are still
        // constellation-inert.

        large_splitter_iter_NewBotSt = m_BLC_transitions.data_end();
        large_splitter_iter_end_NewBotSt = m_BLC_transitions.data_end();
        large_splitter_is_known_to_be_a_strict_BLC_set = true;
      }

      /* 2. If the block does not contain non-bottom states, all states have */ assert(bi.start_bottom_states==bri.start_bottom_states[ReachAlw]);
      /*    been distributed.  Finalize the refinement and return.  (There   */ assert(bri.start_bottom_states[ReachAlw]<=bri.start_bottom_states[AvoidSml]);
      /*    may be up to three subblocks, namely ReachAlw/AvoidSml/AvoidLrg. */ assert(bri.start_bottom_states[AvoidSml]<=bri.start_bottom_states[AvoidLrg]);
      /*    Pick the first and the last subblock and split off the smaller   */ assert(bri.start_bottom_states[AvoidLrg]<=bri.start_bottom_states[AvoidLrg+1]);
      /*    of the two.  Then compare the remaining two subblocks and again  */ assert(bri.start_bottom_states[AvoidLrg+1]==bi.sta.rt_non_bottom_states);
      //    split off the smaller one.)
      if (bi.sta.rt_non_bottom_states==bi.end_states)
      {
        // Algorithm 3, Line 3.31–3.32
        block_type_lb* ReachAlw_block_index = null_block_lb;
        constellation_type_lb& constellation=*bi.constellation;
        const bool constellation_was_trivial=
                  constellation.start_const_states->ref_state->block==
                  std::prev(constellation.end_const_states)->ref_state->block;
        bool constellation_becomes_nontrivial=false;
        const state_index half_orig_bi_size = number_of_states_in_block(bi)/2;
        // Algorithm 3, Line 3.39
        if (bri.bottom_size(ReachAlw) < bri.bottom_size(AvoidLrg))
        {                                                                       assert(bi.start_bottom_states==bri.start_bottom_states[ReachAlw]);
          if (0 < bri.bottom_size(ReachAlw))
          {
            bi.start_bottom_states = bri.start_bottom_states[ReachAlw+1];
            ReachAlw_block_index=create_new_block
                                     (bri.start_bottom_states[ReachAlw],
                                      bri.start_bottom_states[ReachAlw+1],
                                      bri.start_bottom_states[ReachAlw+1], bi);
            constellation_becomes_nontrivial=true;
          }
          if (bri.bottom_size(AvoidSml) < bri.bottom_size(AvoidLrg))
          {                                                                     assert(bi.start_bottom_states==bri.start_bottom_states[AvoidSml]);
            if (0 < bri.bottom_size(AvoidSml))
            {
              bi.start_bottom_states = bri.start_bottom_states[AvoidSml+1];
              create_new_block(bri.start_bottom_states[AvoidSml],
                               bri.start_bottom_states[AvoidSml+1],
                               bri.start_bottom_states[AvoidSml+1], bi);
              constellation_becomes_nontrivial=true;
            }
          }
          else if (0 < bri.bottom_size(AvoidLrg))
          {                                                                     assert(bi.end_states==bri.start_bottom_states[AvoidLrg+1]);
            bi.sta.rt_non_bottom_states = bri.start_bottom_states[AvoidLrg];
            bi.end_states = bri.start_bottom_states[AvoidLrg];
            create_new_block(bri.start_bottom_states[AvoidLrg],
                             bri.start_bottom_states[AvoidLrg+1],
                             bri.start_bottom_states[AvoidLrg+1], bi);
            constellation_becomes_nontrivial=true;
          }
        }
        else
        {                                                                       assert(bi.end_states==bri.start_bottom_states[AvoidLrg+1]);
          if (0 < bri.bottom_size(AvoidLrg))
          {
            bi.sta.rt_non_bottom_states = bri.start_bottom_states[AvoidLrg];
            bi.end_states = bri.start_bottom_states[AvoidLrg];
            create_new_block(bri.start_bottom_states[AvoidLrg],
                             bri.start_bottom_states[AvoidLrg+1],
                             bri.start_bottom_states[AvoidLrg+1], bi);
            constellation_becomes_nontrivial=true;
          }
          if (bri.bottom_size(ReachAlw) < bri.bottom_size(AvoidSml))
          {                                                                     assert(bi.start_bottom_states==bri.start_bottom_states[ReachAlw]);
            bi.start_bottom_states = bri.start_bottom_states[ReachAlw+1];       assert(0<bri.bottom_size(ReachAlw));
            ReachAlw_block_index=create_new_block
                                     (bri.start_bottom_states[ReachAlw],
                                      bri.start_bottom_states[ReachAlw+1],
                                      bri.start_bottom_states[ReachAlw+1], bi);
            constellation_becomes_nontrivial=true;
          }
          else
          {
            ReachAlw_block_index = &bi;
            if (0 < bri.bottom_size(AvoidSml))
            {                                                                   assert(bi.end_states==bri.start_bottom_states[AvoidSml+1]);
              bi.sta.rt_non_bottom_states = bri.start_bottom_states[AvoidSml];
              bi.end_states = bri.start_bottom_states[AvoidSml];
              create_new_block(bri.start_bottom_states[AvoidSml],
                               bri.start_bottom_states[AvoidSml+1],
                               bri.start_bottom_states[AvoidSml+1], bi);
              constellation_becomes_nontrivial=true;
            }
          }
        }

        if (constellation_becomes_nontrivial && constellation_was_trivial)
        {                                                                       assert(std::find(m_non_trivial_constellations.begin(),
          /* This constellation was trivial, as it will be split add it to   */                  m_non_trivial_constellations.end(),
          /* the non-trivial constellations.                                 */                  &constellation)==m_non_trivial_constellations.end());
          m_non_trivial_constellations.emplace_back(&constellation);
        }
        // Algorithm 3, Line 3.43
        if (half_orig_bi_size >= number_of_states_in_block(bi))
        {
          // Every new sub-block is small anyway, but in this situation the old
          // block has diminished in size so much that it is a small subset of
          // its former extent.
          bi.is_small_subblock = true;
        }
        return ReachAlw_block_index;
      }                                                                         assert(m_branching);

      // 3. We distinguish situations where some of these subblocks are empty:
      //    - If there are no AvoidSml-bottom states, then AvoidSml will be
      //      empty.
      //    - It may also happen that there are no AvoidLrg-bottom states but
      //      there are ReachAlw-bottom states because every bottom state with
      //      a transition in the main splitter also has a transition in the
      //      co-splitter; then it is clear from the start that AvoidLrg is
      //      empty.  Potential-AvoidLrg non-bottom states are in NewBotSt
      //      instead.
      //    - It may be that there are no ReachAlw-bottom states but there are
      //      AvoidLrg-bottom states because no bottom state with a transition
      //      in the main splitter has a transition in the co-splitter; then it
      //      is clear from the start that ReachAlw is empty.
      //      Potential-ReachAlw non-bottom states are in NewBotSt instead.
      //    Empty subblocks are considered finished.

      // 4. We decide whether one of the subblocks is already too large (more
      //    than 50% of the unfinished states); if yes, this subblock is
      //    immediately aborted.  At most one subblock can be aborted at any
      //    time.  The aborted subblock is *not* considered finished.
      /*    (We use variable `no_of_unfinished_states_in_block` to record the*/ assert(non_bottom_states[ReachAlw].empty());
      /*    number of unfinished states as long as there is no aborted       */ assert(non_bottom_states[AvoidSml].empty());
      /*    subblock; as soon as a subblock is aborted, it is set to the     */ assert(non_bottom_states[AvoidLrg].empty());
      /*    largest possible value to avoid aborting another subblock.)      */ assert(non_bottom_states_NewBotSt.empty());

      enum { state_checking,
             incoming_inert_transition_checking,
             outgoing_constellation_checking,
             aborted, finished } status[3], status_NewBotSt;
      state_in_block_pointer_lb* current_bottom_state_iter[3];

      // the number of states in the block that are not yet in finished
      // subblocks; but if some process has been aborted already, it is equal
      // to `std::numeric_limits<state_index>::max()`:
      state_index no_of_unfinished_states_in_block=
                                                 number_of_states_in_block(bi);

      /// \brief Abort if there are too many bottom states in a subblock, used before the coroutines start
      /// \details This macro applies to ReachAlw, AvoidSml, or AvoidLrg.
      ///
      /// If the bottom states alone already cover more than half of
      /// a block, the corresponding coroutine does not need to start.
      /// The macro returns true if the coroutine is aborted.
      #define abort_if_bottom_size_too_large(coroutine)                                                                                        \
          ((                                                                    assert(non_bottom_states[(coroutine)].empty()),                \
            bri.bottom_size((coroutine))>no_of_unfinished_states_in_block/2) &&                                                                \
           (/* Algorithm 3, Line 3.8                                         */ assert(std::numeric_limits<state_index>::max()!=               \
                                                                                                            no_of_unfinished_states_in_block), \
            no_of_unfinished_states_in_block=                                                                                                  \
                                       std::numeric_limits<state_index>::max(), assert(m_aut.num_states()<no_of_unfinished_states_in_block/2), \
            status[(coroutine)]=aborted,                                                                                                       \
            true))

      /// \brief Abort if there are too many states in subblock NewBotSt
      /// \details: If the states, possibly after adding i additional states,
      /// cover more than half of the states in the unfinished subblocks,
      /// NewBotSt can be aborted.  The parameter i allows to apply the test
      /// even before adding a state, to avoid storing data that is immediately
      /// going to be abolished.
      ///
      /// NewBotSt has only non-bottom states, so we need a macro that
      /// is different from the other subblocks.
      ///
      /// This macro can be used before the coroutines start or while they run.
      /// The macro returns true if the coroutine is aborted.
      #define abort_if_non_bottom_size_too_large_NewBotSt(i)                                                                                   \
          ((                                                                    assert(aborted!=status_NewBotSt),                              \
            non_bottom_states_NewBotSt.size()+(i)>                                                                                             \
                                         no_of_unfinished_states_in_block/2) &&                                                                \
           (/* Algorithm 3, Line 3.8                                         */ assert(std::numeric_limits<state_index>::max()!=               \
                                                                                                            no_of_unfinished_states_in_block), \
            no_of_unfinished_states_in_block=                                                                                                  \
                                       std::numeric_limits<state_index>::max(), assert(m_aut.num_states()<no_of_unfinished_states_in_block/2), \
            status_NewBotSt=aborted,                                                                                                           \
            true))

      /// \brief Abort if there are too many states in a subblock
      /// \details: If the states, possibly after adding i additional states,
      /// cover more than half of the states in the unfinished subblocks, the
      /// coroutine can be aborted.  The parameter i allows to apply the test
      /// even before adding a state, to avoid storing data that is immediately
      /// going to be abolished.
      ///
      /// If the coroutine is aborted, its non-bottom state vector is
      /// immediately cleared, as it is of no use any more.  (Marked counters
      /// can be found through `potential_non_bottom_states`.)
      ///
      /// This macro can be used while the coroutines run.
      /// The macro returns true if the coroutine is aborted.
      #define abort_if_size_too_large(coroutine, i)                                                                                            \
          (bottom_and_non_bottom_size((coroutine))+(i)>                                                                                        \
                                          no_of_unfinished_states_in_block/2 &&                                                                \
           (/* Algorithm 3, Line 3.8                                         */ assert(std::numeric_limits<state_index>::max()!=               \
                                                                                                            no_of_unfinished_states_in_block), \
            no_of_unfinished_states_in_block=                                                                                                  \
                                       std::numeric_limits<state_index>::max(), assert(m_aut.num_states()<no_of_unfinished_states_in_block/2), \
            status[(coroutine)]=aborted,                                                                                                       \
            non_bottom_states[(coroutine)].clear(),                                                                                            \
            true))

      int no_of_finished_searches=0;      // including the NewBotSt-search
      int no_of_running_searches=0;       // does not include the NewBotSt-search
      enum subblocks running_searches[3]; // does not include the NewBotSt-search

      if (0==bri.bottom_size(AvoidSml))
      {                                                                         assert(0==bri.bottom_size(AvoidSml));
        // Algorithm 3, Line 3.30 left (AvoidSml)
        /* AvoidSml is empty and finishes early.  There are no states that   */ assert(bri.potential_non_bottom_states[AvoidSml].empty());
        // might be moved to NewBotSt.
        if (0==bri.bottom_size(AvoidLrg))
        {
          // Algorithm 3, Lines 3.30-3.32 left (AvoidLrg)
          //++no_of_finished_searches;
          //status_NewBotSt=finished;
          // This is a trivial split and nothing needs to be done.
          // If AvoidLrg were not yet finished, it could still happen that
          // some states are found to have a transition in the co-splitter,
          // so they would yet be added to NewBotSt.

          clear_state_counters
                         (bri.potential_non_bottom_states[ReachAlw].begin(),
                          bri.potential_non_bottom_states[ReachAlw].end(), bi);
          clear(bri.potential_non_bottom_states[ReachAlw]);
          clear_state_counters
                          (bri.potential_non_bottom_states_HitSmall.begin(),
                           bri.potential_non_bottom_states_HitSmall.end(), bi);
          clear(bri.potential_non_bottom_states_HitSmall);
          // Algorithm 3, Line 3.43
          return &bi;
        }
        ++no_of_finished_searches;
        status[AvoidSml]=finished;
      }
      else if (!abort_if_bottom_size_too_large(AvoidSml))
      {
        running_searches[no_of_running_searches] = AvoidSml;
        ++no_of_running_searches;
        current_bottom_state_iter[AvoidSml]=bri.start_bottom_states[AvoidSml];
        status[AvoidSml]=state_checking;
      }

      if (0 == bri.bottom_size(AvoidLrg))
      {
        // Algorithm 3, Line 3.30 left (AvoidLrg)
        /* AvoidLrg is empty and finishes early.                             */ assert(bri.potential_non_bottom_states[AvoidLrg].empty());
        ++no_of_finished_searches;
        status[AvoidLrg]=finished;
      }
      else if (!abort_if_bottom_size_too_large(AvoidLrg))
      {
        running_searches[no_of_running_searches] = AvoidLrg;
        ++no_of_running_searches;
        current_bottom_state_iter[AvoidLrg]=bri.start_bottom_states[AvoidLrg];
        status[AvoidLrg]=state_checking;
      }

      status_NewBotSt=state_checking;
      if (0==bri.bottom_size(ReachAlw))
      {
        // Algorithm 3, Line 3.30 left (ReachAlw)
        // ReachAlw is empty and finishes early.  Its non-bottom states are
        // actually in NewBotSt (because they can inertly reach a AvoidLrg- or
        /* AvoidSml-bottom-state).                                           */ assert(non_bottom_states_NewBotSt.empty());
        // Algorithm 3, Line 3.33 left (ReachAlw)
        non_bottom_states_NewBotSt.swap_vec
                                   (bri.potential_non_bottom_states[ReachAlw]);
        if (finished == status[AvoidLrg])
        {
          // Algorithm 3, Line 2.34-3.35 left (ReachAlw)
          // both ReachAlw and AvoidLrg are empty.  So the HitSmall states must
          // be in NewBotSt.  (NewBotSt has not yet been aborted.)
          if (!non_bottom_states_NewBotSt.empty())
          {
            non_bottom_states_NewBotSt.add_todo
                             (bri.potential_non_bottom_states_HitSmall.begin(),
                              bri.potential_non_bottom_states_HitSmall.end());
            clear(bri.potential_non_bottom_states_HitSmall);
          }
          else
          {
            non_bottom_states_NewBotSt.swap_vec
                                    (bri.potential_non_bottom_states_HitSmall);
          }
        }
        for (state_in_block_pointer_lb st: non_bottom_states_NewBotSt)
        {                                                                       // The work can be assigned to the same main splitter transition(s) that made
                                                                                // the state get into ReachAlw (depending on whether the source or target
          st.ref_state->counter=marked_NewBotSt;                                // constellation are new, see above).
        }
        ++no_of_finished_searches;
        status[ReachAlw]=finished;
        abort_if_non_bottom_size_too_large_NewBotSt(0);
      }
      else if (!abort_if_bottom_size_too_large(ReachAlw))
      {
        running_searches[no_of_running_searches] = ReachAlw;
        ++no_of_running_searches;
        current_bottom_state_iter[ReachAlw]=bri.start_bottom_states[ReachAlw];
        status[ReachAlw]=state_checking;
      }

      // 5. We start the coroutines for the non-empty, non-aborted subblocks.
      //    Every coroutine executes one step in turn.  The coroutines stop as
      //    soon as three of them have finished (including empty subblocks).
      //    Generally the X-coroutine finds predecessors of states that are
      //    determined to be in the X-subblock and adds them first to the
      //    potentially-X states; as soon as every successor of a state is
      //    known to be in the X-subblock, the state is determined to be in the
      //    X-subblock itself.
      //    There are two twists here:
      //    - The coroutine for the AvoidLrg-subblock needs to check, when all
      //      successors are known to be in the AvoidLrg-subblock, whether the
      //      state has a transition in the co-splitter; if yes, the state is
      //      actually a new bottom state in the NewBotSt-subblock (all its
      //      inert successors are in AvoidLrg but the state itself is in
      //      NewBotSt).
      //    - Predecessors of NewBotSt-states are immediately added to the
      //      NewBotSt-subblock because for them, having one NewBotSt-successor
      //      is enough.  There is no set of potentially-NewBotSt states.

      std::vector<transition>::iterator current_source_iter[3];
      std::vector<transition>::iterator current_source_iter_NewBotSt;
      std::vector<transition>::const_iterator current_source_iter_end[3];
      std::vector<transition>::const_iterator current_source_iter_end_NewBotSt;

      state_in_block_pointer_lb current_source_AvoidLrg;
      outgoing_transitions_const_it_lb
          current_outgoing_iter_start_AvoidLrg, current_outgoing_iter_AvoidLrg; assert(large_splitter_iter_NewBotSt<=large_splitter_iter_end_NewBotSt);
      for (;;)
      {                                                                         assert(2>=no_of_finished_searches);
        for (int current_search_index=0; current_search_index<
                                no_of_running_searches; ++current_search_index)
        {
          const enum subblocks
                         current_search=running_searches[current_search_index]; assert(0<=current_search);  assert(current_search<NewBotSt);

          if (incoming_inert_transition_checking==status[current_search])
          {                                                                     assert(current_source_iter[current_search]<
            /* Algorithm 3, Line 3.11 left                                   */                                       current_source_iter_end[current_search]);
                                                                                mCRL2complexity(&m_transitions[std::distance(m_aut.get_transitions().begin(),
                                                                                        current_source_iter[current_search])], add_work(check_complexity::
                                                                                                     simple_splitB_U__handle_transition_to_U_state, 1), *this);
            const transition& tr=*current_source_iter[current_search]++;        assert(m_aut.is_tau(m_aut_apply_hidden_label_map(tr.label())));
            state_in_block_pointer_lb const src = m_states.begin() + tr.from(); assert(m_states[tr.to()].block==&bi);
            // Algorithm 3, Line 3.12 left
            if (src.ref_state->block==&bi &&
                !(m_preserve_divergence && tr.from()==tr.to()))
            {                                                                   assert(!non_bottom_states[ReachAlw].find(src));
                                                                                assert(!non_bottom_states[AvoidSml].find(src));
                                                                                assert(!non_bottom_states[AvoidLrg].find(src));
              const transition_index current_counter=src.ref_state->counter;
              // Algorithm 3, Line 3.14–3.15 left
              if(  (   (   undefined==current_counter
                        || (   marked_HitSmall==current_counter
                            && AvoidSml!=current_search        )                || (assert(marked_HitSmall!=current_counter || AvoidSml==current_search),false)
                                                                )
                    && (// Algorithm 3, Line 3.20 left
                        src.ref_state->counter=marked(current_search)+
                        src.ref_state->no_of_outgoing_block_inert_transitions,  assert(std::find(bri.potential_non_bottom_states[current_search].begin(),
                                                                                                 bri.potential_non_bottom_states[current_search].end(), src)==
                        /* Algorithm 3, Line 3.19 left                       */                         bri.potential_non_bottom_states[current_search].end()),
                        bri.potential_non_bottom_states[current_search].
                                                             push_back(src),
                        true                                                ))
                 || is_in_marked_range_of(current_counter, current_search)    )
              {                                                                 assert(is_in_marked_range_of(src.ref_state->counter, current_search));
                // Algorithm 3, Line 3.21 left
                --src.ref_state->counter;                                       assert(is_in_marked_range_of(src.ref_state->counter, current_search));
                /* Algorithm 3, Line 3.22–3.23 left                          */ assert(!non_bottom_states_NewBotSt.find(src));
                if (marked(current_search)==src.ref_state->counter)
                {
                  // all inert transitions of src point to the current subblock
                  // Algorithm 3, Line 3.24 left
                  if (AvoidLrg==current_search &&
                      large_splitter_iter_NewBotSt!=
                                              large_splitter_iter_end_NewBotSt)
                  {                                                             assert(nullptr != bri.large_splitter);
                    // but AvoidLrg needs to check whether src has a transition
                    // in the large splitter
                    // (This can be avoided if we remember that the state had
                    // been in HitSmall earlier; then we know that it had a
                    // transition in the small splitter but none in the
                    // large splitter.)
                    current_source_AvoidLrg=src;
                    status[AvoidLrg] = outgoing_constellation_checking;
                    current_outgoing_iter_start_AvoidLrg=
                                     src.ref_state->start_outgoing_transitions;
                    current_outgoing_iter_AvoidLrg=
                      std::next(src.ref_state)>=m_states.end()
                        ? m_outgoing_transitions.end()
                        : std::next(src.ref_state)->start_outgoing_transitions; assert(current_outgoing_iter_start_AvoidLrg<current_outgoing_iter_AvoidLrg);
                    continue;
                  }
                  // Algorithm 3, Line 3.8
                  if (abort_if_size_too_large(current_search, 1))
                  {                                                             assert(running_searches[current_search_index]==current_search);
                    --no_of_running_searches;                                   assert(current_search_index<=no_of_running_searches);
                    running_searches[current_search_index]=
                                      running_searches[no_of_running_searches]; assert(std::find(bri.potential_non_bottom_states[current_search].begin(),
                                                                                                 bri.potential_non_bottom_states[current_search].end(), src)!=
                                                                                                 bri.potential_non_bottom_states[current_search].end());
                    --current_search_index;
                    continue;
                  }
                  // Algorithm 3, Line 3.29 left
                  non_bottom_states[current_search].add_todo(src);
                }
              }
              // Algorithm 3, Line 3.16 left
              else if (marked_NewBotSt != src.ref_state->counter)
              {
                // The state has block-inert transitions to multiple
                // subblocks (or it is HitSmall and the current search is
                /* AvoidSml).  It should be added to NewBotSt.               */ assert(!non_bottom_states_NewBotSt.find(src));
                // Algorithm 3, Line 3.17 left
                if (aborted!=status_NewBotSt &&
                    !abort_if_non_bottom_size_too_large_NewBotSt(1))
                {
                  // but actually if NewBotSt is already aborted, there is no
                  // need to add the state to NewBotSt.  (If the current search
                  // ends first or second, the state will be added to NewBotSt
                  // later anyway, but if the current search ends as third we
                  // have saved the assignment.)
                  src.ref_state->counter = marked_NewBotSt;
                  non_bottom_states_NewBotSt.add_todo(src);
                }
              }                                                                 else {assert(aborted==status_NewBotSt||non_bottom_states_NewBotSt.find(src));}
            }

            if (current_source_iter[current_search]!=
                                     current_source_iter_end[current_search] &&
                m_aut.is_tau(m_aut_apply_hidden_label_map
                               (current_source_iter[current_search]->label())))
            {
              continue;
            }
            status[current_search]=state_checking;
          }
          else if (state_checking == status[current_search])
          {
            // Algorithm 3, Line 3.11 left
            state_in_block_pointer_lb const tgt=
                    current_bottom_state_iter[current_search]<
                                      bri.start_bottom_states[current_search+1]
                          ? *current_bottom_state_iter[current_search]++
                          : non_bottom_states[current_search].move_from_todo(); assert(!non_bottom_states[current_search^1].find(tgt));

            /* Prepare for the sources of tgt to be added to the subblock    */ mCRL2complexity(tgt.ref_state,
                                                                                     add_work(check_complexity::simple_splitB_U__find_predecessors, 1), *this);
            current_source_iter[current_search]=
                                     tgt.ref_state->start_incoming_transitions; assert(!non_bottom_states[current_search^2].find(tgt));
            current_source_iter_end[current_search]=
                  std::next(tgt.ref_state)>=m_states.end()
                        ? m_aut.get_transitions().end()
                        : std::next(tgt.ref_state)->start_incoming_transitions; assert(!non_bottom_states[current_search^3].find(tgt));
            if (current_source_iter[current_search]<
                                     current_source_iter_end[current_search] &&
                m_aut.is_tau(m_aut_apply_hidden_label_map
                               (current_source_iter[current_search]->label())))
            {
              status[current_search]=incoming_inert_transition_checking;
              continue;
            }
          }
          else
          {                                                                     assert(AvoidLrg==current_search);
            /* Algorithm 3, Line 3.25 left (AvoidLrg)                        */ assert(outgoing_constellation_checking==status[AvoidLrg]);
                                                                                assert(current_outgoing_iter_start_AvoidLrg<current_outgoing_iter_AvoidLrg);
                                                                                assert(m_outgoing_transitions.end()==current_outgoing_iter_AvoidLrg ||
                                                                                       current_outgoing_iter_start_AvoidLrg<
                                                                                                               current_outgoing_iter_AvoidLrg->start_same_saC);
            --current_outgoing_iter_AvoidLrg;                                   assert(current_outgoing_iter_AvoidLrg->start_same_saC<=
                                                                                                                                current_outgoing_iter_AvoidLrg);
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  // Assign the work to the transitions in the same_saC slice
                                                                                  outgoing_transitions_const_it_lb out_it=
                                                                                                                current_outgoing_iter_AvoidLrg->start_same_saC;
                                                                                  mCRL2complexity(&m_transitions[*out_it->ref_BLC_transitions],
                                                                                        add_work(check_complexity::
                                                                                         simple_splitB_U__handle_transition_from_potential_U_state, 1), *this);
                                                                                  #ifndef NDEBUG
                                                                                    while (++out_it<=current_outgoing_iter_AvoidLrg) {
                                                                                      mCRL2complexity(&m_transitions[*out_it->ref_BLC_transitions],
                                                                                         add_work_notemporary(check_complexity::
                                                                                         simple_splitB_U__handle_transition_from_potential_U_state, 1), *this);
                                                                                    }
                                                                                  #endif
                                                                                #endif
                                                                                assert(!non_bottom_states[ReachAlw].find(current_source_AvoidLrg));
                                                                                assert(!non_bottom_states[AvoidLrg].find(current_source_AvoidLrg));
                                                                                assert(!non_bottom_states[AvoidSml].find(current_source_AvoidLrg));
                                                                                assert(marked(AvoidLrg)==current_source_AvoidLrg.ref_state->counter ||
            /* Algorithm 3, Line 3.26 left (AvoidLrg)                        */        marked_NewBotSt==current_source_AvoidLrg.ref_state->counter);
            simple_list<BLC_indicators_lb>::const_iterator const
                current_splitter=m_transitions[
                      *current_outgoing_iter_AvoidLrg->ref_BLC_transitions].
                                        transitions_per_block_to_constellation; assert(nullptr != bri.large_splitter);
            if (current_splitter==bri.large_splitter)
            {
              // The state has a transition in the large splitter, so it should
              // not be added to AvoidLrg.  Instead, add it to NewBotSt:
              // Algorithm 3, Line 3.27 left (AvoidLrg)
              if (marked_NewBotSt!=current_source_AvoidLrg.ref_state->counter)
              {
                // It doesn't happen often that the source is marked NewBotSt
                // exactly while AvoidLrg is running this search -- so we do
                /* not test this very often.                                 */ assert(!non_bottom_states_NewBotSt.find(current_source_AvoidLrg));
                if (aborted!=status_NewBotSt &&
                    !abort_if_non_bottom_size_too_large_NewBotSt(1))
                {                                                               assert(aborted!=status_NewBotSt);
                  // but actually if NewBotSt is already aborted, there is no
                  // need to add the state to NewBotSt.  (If the current search
                  // ends first or second, the state will be added to NewBotSt
                  // later anyway, but if the current search ends as third we
                  // have saved the assignment.)
                  current_source_AvoidLrg.ref_state->counter = marked_NewBotSt;
                  non_bottom_states_NewBotSt.add_todo(current_source_AvoidLrg);
                }
              }                                                                 else  {  assert(non_bottom_states_NewBotSt.find(current_source_AvoidLrg));  }
            }
            else if (current_outgoing_iter_AvoidLrg=
                                current_outgoing_iter_AvoidLrg->start_same_saC,
                     current_outgoing_iter_start_AvoidLrg==
                                             current_outgoing_iter_AvoidLrg
                      // We have searched all outgoing transitions but found
                      // none in the co-splitter
                      // (We tried several options to accelerate this test,
                      // e.g. remembering whether `current_source_AvoidLrg`
                      // had been in HitSmall earlier; letting the
                      // NewBotSt-coroutine go through the co-splitter
                      // transitions instead of only waiting to mark them
                      // as "cannot be in AvoidLrg"; even just comparing
                      // `current_splitter==small_splitter`.  But none of these
                      // options would have much effect, so we decided to stick
                      // with the simpler code.)
                                                                           )
            {                                                                   assert(marked(AvoidLrg)==current_source_AvoidLrg.ref_state->counter);
              // Algorithm 3, Line 3.8
              if(abort_if_size_too_large(AvoidLrg,1))
              {                                                                 assert(running_searches[current_search_index]==AvoidLrg);
                --no_of_running_searches;                                       assert(current_search_index<=no_of_running_searches);
                running_searches[current_search_index]=
                                      running_searches[no_of_running_searches]; assert(std::find(bri.potential_non_bottom_states[AvoidLrg].begin(),
                                                                                        bri.potential_non_bottom_states[AvoidLrg].end(), current_source_AvoidLrg)!=
                                                                                        bri.potential_non_bottom_states[AvoidLrg].end());
                --current_search_index;
                continue;
              }
              // Algorithm 3, Line 3.29 left
              non_bottom_states[AvoidLrg].add_todo(current_source_AvoidLrg);
            }
            else
            {
              continue;
            }
            // At this point the search for outgoing transitions has finished
            // (and AvoidLrg is still running). We can go back to the previous
            // status.
            if (current_source_iter[AvoidLrg]!=
                 current_source_iter_end[AvoidLrg] &&
                m_aut.is_tau(m_aut_apply_hidden_label_map(current_source_iter
                                                         [AvoidLrg]->label())))
            {
              status[AvoidLrg] = incoming_inert_transition_checking;
              continue;
            }
            status[AvoidLrg]=state_checking;
          }

          /* Now we have done one step in the handling of this subblock.  If */ assert(state_checking==status[current_search]);
          /* we reach this point, it is time to check whether the subblock is*/ assert(NewBotSt!=current_search);
          // finished.
          if (current_bottom_state_iter[current_search]==
                                   bri.start_bottom_states[current_search+1] &&
              non_bottom_states[current_search].todo_is_empty())
          {
            // the current search is completed. Finish the subblock:
            // Algorithm 3, Line 3.30 left
            status[current_search]=finished;
            ++no_of_finished_searches;
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  // Finalise the work distribution here:
                                                                                  // Forget the balance of earlier processes that finished:
                                                                                  // (If NewBotSt is unfinished, the third process does enough work to tilt the
                                                                                  // balance into the positive.  If another process is unfinished, then
                                                                                  // NewBotSt and the last process that finished before NewBotSt together
                                                                                  // should provide enough credit.)
                                                                                  check_complexity::check_temporary_work();
                                                                                  // move the work from temporary state counters to final ones
                                                                                  const unsigned char max_new_B=check_complexity::log_n-
                                                                                           check_complexity::ilog2(bottom_and_non_bottom_size(current_search));
                                                                                  for (const state_in_block_pointer_lb* s=bri.start_bottom_states[current_search];
                                                                                                       (s!=bri.start_bottom_states[current_search+1] ||
                                                                                                        (s=non_bottom_states[current_search].data(), true)) &&
                                                                                                       s!=non_bottom_states[current_search].data_end(); ++s)
                                                                                  {
                                                                                    mCRL2complexity(s->ref_state, finalise_work(check_complexity::
                                                                                        simple_splitB_U__find_predecessors, check_complexity::
                                                                                          simple_splitB__find_predecessors_of_R_or_U_state, max_new_B), *this);
                                                                                    // incoming tau-transitions of s
                                                                                    const std::vector<transition>::const_iterator in_ti_end=
                                                                                        std::next(s->ref_state)>=m_states.end() ? m_aut.get_transitions().end()
                                                                                                         : std::next(s->ref_state)->start_incoming_transitions;
                                                                                    for (std::vector<transition>::const_iterator
                                                                                              ti=s->ref_state->start_incoming_transitions; ti!=in_ti_end; ++ti)
                                                                                    {
                                                                                      if (!m_aut.is_tau(m_aut_apply_hidden_label_map(ti->label()))) { break; }
                                                                                      mCRL2complexity(&m_transitions[std::distance(m_aut.get_transitions().
                                                                                                           cbegin(), ti)], finalise_work(check_complexity::
                                                                                          simple_splitB_U__handle_transition_to_U_state, check_complexity::
                                                                                          simple_splitB__handle_transition_to_R_or_U_state, max_new_B), *this);
                                                                                    }
                                                                                    if (AvoidLrg==current_search &&
                                                                                        0!=s->ref_state->no_of_outgoing_block_inert_transitions)
                                                                                    {
                                                                                      // outgoing transitions of s
                                                                                      const outgoing_transitions_const_it_lb out_ti_end=
                                                                                         std::next(s->ref_state)>=m_states.end() ? m_outgoing_transitions.end()
                                                                                                         : std::next(s->ref_state)->start_outgoing_transitions;
                                                                                      for (outgoing_transitions_const_it_lb
                                                                                             ti=s->ref_state->start_outgoing_transitions; ti!=out_ti_end; ++ti)
                                                                                      {
                                                                                        mCRL2complexity(&m_transitions[*ti->ref_BLC_transitions],
                                                                                          finalise_work(check_complexity::
                                                                                                  simple_splitB_U__handle_transition_from_potential_U_state,
                                                                                            check_complexity::
                                                                                                  simple_splitB__handle_transition_from_R_or_U_state,
                                                                                                                                            max_new_B), *this);
                                                                                      }
                                                                                    }
                                                                                  }
                                                                                  if (AvoidLrg==current_search)
                                                                                  {
                                                                                    // Also handle the work for states that were potentially in AvoidLrg but
                                                                                    // turned out to be new bottom states.  The states that ended up actually
                                                                                    // in AvoidLrg have already been handled above.  We just go over all states
                                                                                    // again, as only the non-AvoidLrg-states have the relevant counter !=0.
                                                                                    // (We cannot only go over non_bottom_states_NewBotSt because some states
                                                                                    // may have been handled by AvoidLrg after NewBotSt became too large.)
                                                                                    for (const state_in_block_pointer_lb*
                                                                                                          s=bi.sta.rt_non_bottom_states; s!=bi.end_states; ++s)
                                                                                    {
                                                                                      // outgoing transitions of s
                                                                                      const outgoing_transitions_const_it_lb out_ti_end=
                                                                                         std::next(s->ref_state)>=m_states.end() ? m_outgoing_transitions.end()
                                                                                                         : std::next(s->ref_state)->start_outgoing_transitions;
                                                                                      for (outgoing_transitions_const_it_lb
                                                                                             ti=s->ref_state->start_outgoing_transitions; ti!=out_ti_end; ++ti)
                                                                                      {
                                                                                        mCRL2complexity(&m_transitions[*ti->ref_BLC_transitions], finalise_work
                                                                                            (check_complexity::
                                                                                                     simple_splitB_U__handle_transition_from_potential_U_state,
                                                                                             check_complexity::
                                                                                               simple_splitB__test_outgoing_transitions_found_new_bottom_state,
                                                                                             1), *this);
                                                                                        // At this point we have not yet identified the new bottom states,
                                                                                        // so we cannot be more specific than giving ``1'' as the new counter
                                                                                        // value to be assigned if there has been work.  After identifying the
                                                                                        // new bottom states, we could be more strict and require ``0'' in
                                                                                        // states that are still non-bottom.
                                                                                      }
                                                                                    }
                                                                                  }
                                                                                #endif
            // Algorithm 3, Line 3.31 left
            if (3>no_of_finished_searches)
            {
              // Algorithm 3, Line 3.33 left
              /* If NewBotSt is not empty, then the following reserve() call */ assert(finished!=status_NewBotSt);
              // would reserve an overapproximation of the needed space,
              // because some states likely have moved from current_search to
              // NewBotSt already.  Therefore I do not include it.  Only if
              // NewBotSt.size() is less than what is added to it there *may*
              // be multiple reallocations.  If NewBotSt.size() is less than
              // 1/3 of what is added to it there *will* be multiple
              // reallocations.
              if (non_bottom_states_NewBotSt.empty())
              {
                non_bottom_states_NewBotSt.reserve
                       (// non_bottom_states_NewBotSt.size()
                        +bri.potential_non_bottom_states[current_search].size()
                        -non_bottom_states[current_search].size());
              }
              for (state_in_block_pointer_lb st:
                               bri.potential_non_bottom_states[current_search])
              {                                                                 // The work in this loop can be assigned to the same transition(s) that made
                                                                                // st go into `potential_non_bottom_states[current_search]`.  (It can now be
                                                                                // a final counter, as we know for sure the subblock is not aborted.)
                if (marked_NewBotSt != st.ref_state->counter)
                {                                                               assert(is_in_marked_range_of(st.ref_state->counter, current_search));
                  if (marked(current_search)!=st.ref_state->counter)
                  {                                                             assert(!non_bottom_states_NewBotSt.find(st));
                    /* We always add state st to non_bottom_states_NewBotSt, */ assert(!non_bottom_states[ReachAlw].find(st));
                    // even if NewBotSt is aborted, because we want to clear
                    // potential_non_bottom_states[current_search].  The
                    // alternative would be to reset the counter to undefined,
                    // but as state st must have an unexplored block-inert
                    // transition to a different subblock, then that subblock
                    // would add it to its own potential_non_bottom_states
                    // later.
                    non_bottom_states_NewBotSt.add_todo(st);                    assert(!non_bottom_states[AvoidLrg].find(st));
                    st.ref_state->counter = marked_NewBotSt;                    assert(!non_bottom_states[AvoidSml].find(st));
                  }                                                             else  {  assert(non_bottom_states[current_search].find(st));  }
                }                                                               else  {  assert(!non_bottom_states[current_search].find(st));  }
              }                                                                 assert(running_searches[current_search_index]==current_search);
              clear(bri.potential_non_bottom_states[current_search]);
              --no_of_running_searches;                                         assert(current_search_index<=no_of_running_searches);
              running_searches[current_search_index]=
                                      running_searches[no_of_running_searches];
              --current_search_index; /* is now -1, 0 or +1 */
              // Algorithm 3, Line 3.34 left
              if (finished==status[ReachAlw] &&
                  finished==status[AvoidLrg] &&
                  aborted!=status_NewBotSt)
              {                                                                 assert(1>=no_of_running_searches);
                // Algorithm 3, Line 3.35 left
                /* The HitSmall states can be assigned to NewBotSt because   */ assert(finished!=status[AvoidSml]);
                /* they cannot be in ReachAlw or AvoidLrg                    */ assert(finished!=status_NewBotSt);
                for (state_in_block_pointer_lb st:
                                      bri.potential_non_bottom_states_HitSmall)
                {                                                               assert(0<st.ref_state->no_of_outgoing_block_inert_transitions);
                                                                                // The work in this loop can be assigned to the same transitions in
                                                                                // the main splitter as the one(s) that made st become a member of
                                                                                // `potential_non_bottom_states_HitSmall`.
                                                                                assert(!non_bottom_states[AvoidSml].find(st));
                  if (marked_HitSmall == st.ref_state->counter)
                  {                                                             assert(!non_bottom_states_NewBotSt.find(st));
                    non_bottom_states_NewBotSt.add_todo(st);                    assert(!non_bottom_states[ReachAlw].find(st));
                    st.ref_state->counter = marked_NewBotSt;                    assert(!non_bottom_states[AvoidLrg].find(st));
                  }                                                             else  {  assert(marked(ReachAlw)==st.ref_state->counter ||
                                                                                                marked(AvoidLrg)==st.ref_state->counter ||
                                                                                                marked_NewBotSt==st.ref_state->counter);  }
                }
                clear(bri.potential_non_bottom_states_HitSmall);
              }
              if (std::numeric_limits<state_index>::max()!=
                                              no_of_unfinished_states_in_block)
              {                                                                 assert(0<no_of_running_searches);  assert(no_of_running_searches<=2);
                /* Algorithm 3, Line 3.8                                     */ assert(aborted!=status[ReachAlw]);  assert(aborted!=status[AvoidLrg]);
                no_of_unfinished_states_in_block-=
                                    bottom_and_non_bottom_size(current_search); assert(aborted!=status[running_searches[0]]);
                /* Try to find out whether some other process needs to be    */ assert(finished!=status[running_searches[0]]);
                /* aborted, now that we have a more strict size bound.       */ assert(aborted!=status[AvoidSml]);  assert(aborted!=status_NewBotSt);
                if (abort_if_size_too_large(running_searches[0], 0))
                {
                  // The if test in the next line is not necessary, as the
                  // result will just be ignored if 1==no_of_running_searches,
                  // because we will have 0==no_of_running_searches after the
                  // decrement a few lines further down.
                  // if (1<no_of_running_searches)
                  // {
                    running_searches[0]=running_searches[1];
                  // }
                  if (0==current_search_index)
                  {
                    --current_search_index;
                  }
                  --no_of_running_searches; // is now 0 or 1
                }
                else if (1<no_of_running_searches && (                          assert(aborted!=status[running_searches[1]]),
                                                                                assert(finished!=status[running_searches[1]]),
                         abort_if_size_too_large(running_searches[1], 0)))
                {
                  // if (1==current_search_index)  { --current_search_index; }
                  // < will be ignored, because the new search index will
                  // then become 1 again, which is >= the number of running
                  // searches, so the inner main loop will be exited anyway.
                  --no_of_running_searches;                                     assert(1==no_of_running_searches);
                }
                else
                {
                  abort_if_non_bottom_size_too_large_NewBotSt(0);
                }
              }
              continue;
            }

            // Algorithm 3, Line 3.32
            /* All three subblocks ReachAlw/AvoidLrg/AvoidSml are finished.  */ assert(finished==status[AvoidSml]);  assert(finished==status[AvoidLrg]);
            /* NewBotSt is unfinished.                                       */ assert(finished==status[ReachAlw]);

            /* Calculate the placement of subblocks:                         */
            state_in_block_pointer_lb* new_start_bottom_states_plus_one[3];
            state_in_block_pointer_lb* new_end_bottom_states_plus_one[2];
            #define new_start_bottom_states(idx) (assert(1<=(idx)), assert((idx)<=3), new_start_bottom_states_plus_one[(idx)-1])
            #define new_end_bottom_states(idx) (assert(1<=(idx)), assert((idx)<=2), new_end_bottom_states_plus_one[(idx)-1])
            #define new_end_bottom_states_NewBotSt (new_start_bottom_states_plus_one[2])

            const state_index half_orig_bi_size =
                                               number_of_states_in_block(bi)/2;
            new_start_bottom_states(ReachAlw+1) =
                               bri.start_bottom_states[ReachAlw+1] +
                                            non_bottom_states[ReachAlw].size();
            new_end_bottom_states(AvoidSml)=
                 new_start_bottom_states(AvoidSml) + bri.bottom_size(AvoidSml);
            new_start_bottom_states(AvoidSml+1)=
                            new_end_bottom_states(AvoidSml)+
                                            non_bottom_states[AvoidSml].size();
            new_end_bottom_states(AvoidLrg)=
                   new_start_bottom_states(AvoidLrg)+bri.bottom_size(AvoidLrg);
            new_end_bottom_states_NewBotSt = new_end_bottom_states(AvoidLrg)+
                                            non_bottom_states[AvoidLrg].size();
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  // Finish the accounting.  First check that there were not too many waiting
                                                                                  // cycles:  (This check may have been done in NewBotSt but we cannot be sure;
                                                                                  // NewBotSt may have been aborted earlier.)
                                                                                  check_complexity::check_waiting_cycles();
                                                                                  // After this check we are no longer allowed to wait, and we are allowed to
                                                                                  // cancel work.
                                                                                  if (nullptr != bri.large_splitter) {
                                                                                    // Cancel work in the whole block. Actually only the work in NewBotSt needs
                                                                                    // to be cancelled, but the states may not yet have moved there.
                                                                                    for (const state_in_block_pointer_lb*
                                                                                                 s=bi.start_bottom_states; s!=bi.sta.rt_non_bottom_states; ++s)
                                                                                    {
                                                                                      // outgoing transitions of s
                                                                                      const outgoing_transitions_it_lb out_ti_end=
                                                                                         std::next(s->ref_state)>=m_states.end() ? m_outgoing_transitions.end()
                                                                                                         : std::next(s->ref_state)->start_outgoing_transitions;
                                                                                      for (outgoing_transitions_it_lb
                                                                                             ti=s->ref_state->start_outgoing_transitions; ti!=out_ti_end; ++ti)
                                                                                      {
                                                                                        mCRL2complexity(&m_transitions[*ti->ref_BLC_transitions],
                                                                                              cancel_work(check_complexity::
                                                                                                      simple_splitB_R__handle_transition_from_R_state), *this);
                                                                                      }
                                                                                    }
                                                                                  }
                                                                                  for (const state_in_block_pointer_lb*
                                                                                                          s=bi.sta.rt_non_bottom_states; s!=bi.end_states; ++s)
                                                                                  {
                                                                                    mCRL2complexity(s->ref_state, cancel_work
                                                                                                (check_complexity::simple_splitB_R__find_predecessors), *this);
                                                                                    // incoming tau-transitions of s
                                                                                    const std::vector<transition>::iterator in_ti_end=
                                                                                        std::next(s->ref_state)>=m_states.end() ? m_aut.get_transitions().end()
                                                                                                         : std::next(s->ref_state)->start_incoming_transitions;
                                                                                    for (std::vector<transition>::iterator
                                                                                              ti=s->ref_state->start_incoming_transitions; ti!=in_ti_end; ++ti)
                                                                                    {
                                                                                      if (!m_aut.is_tau(m_aut_apply_hidden_label_map(ti->label()))) { break; }
                                                                                      mCRL2complexity(&m_transitions[std::distance(m_aut.
                                                                                            get_transitions().begin(), ti)], cancel_work(check_complexity::
                                                                                                        simple_splitB_R__handle_transition_to_R_state), *this);
                                                                                    }
                                                                                    if (nullptr != bri.large_splitter) {
                                                                                      // outgoing transitions of s
                                                                                      const outgoing_transitions_it_lb out_ti_end=
                                                                                         std::next(s->ref_state)>=m_states.end() ? m_outgoing_transitions.end()
                                                                                                         : std::next(s->ref_state)->start_outgoing_transitions;
                                                                                      for (outgoing_transitions_it_lb
                                                                                             ti=s->ref_state->start_outgoing_transitions; ti!=out_ti_end; ++ti)
                                                                                      {
                                                                                        mCRL2complexity(&m_transitions[*ti->ref_BLC_transitions],
                                                                                              cancel_work(check_complexity::
                                                                                                      simple_splitB_R__handle_transition_from_R_state), *this);
                                                                                      }
                                                                                    }
                                                                                  }
                                                                                  // Reset the work balance counters:
            /* Algorithm 3, Line 3.37–3.38                                   */   check_complexity::check_temporary_work();
                                                                                #endif
            /* As we have aborted the largest block early, it cannot happen  */ assert(new_end_bottom_states_NewBotSt!=bi.end_states);
            // that NewBotSt is empty but aborted.

            constellation_type_lb& constellation=*bi.constellation;
            if (constellation.start_const_states->ref_state->block==
                   std::prev(constellation.end_const_states)->ref_state->block)
            {                                                                   assert(std::find(m_non_trivial_constellations.begin(),
              /* This constellation was trivial, as it will be split add it  */                  m_non_trivial_constellations.end(),
              /* to the non-trivial constellations.                          */                  &constellation)==m_non_trivial_constellations.end());
              m_non_trivial_constellations.emplace_back(&constellation);
            }

            // Algorithm 3, Line 3.39
            // Split off NewBotSt -- actually just make *bi smaller
            block_type_lb& NewBotSt_block_index = bi;
            bi.start_bottom_states=new_end_bottom_states_NewBotSt;              assert(bi.start_bottom_states<bi.end_states);
            bi.sta.rt_non_bottom_states=new_end_bottom_states_NewBotSt;
            // We have to clear state counters of the current search because
            // some of these states may be actually NewBotSt-states that have
            // not yet been identified as such:
            clear_state_counters
                   (bri.potential_non_bottom_states[current_search].begin(),
                    bri.potential_non_bottom_states[current_search].end(), bi);
            clear(bri.potential_non_bottom_states[current_search]);             assert(bri.potential_non_bottom_states[ReachAlw].empty());
            /* The other processes have finished earlier and transferred     */ assert(bri.potential_non_bottom_states[AvoidLrg].empty());
            /* their states in potential_non_bottom_states to NewBotSt.      */ assert(bri.potential_non_bottom_states[AvoidSml].empty());
            clear_state_counters(non_bottom_states_NewBotSt.begin(),
                                 non_bottom_states_NewBotSt.end(), bi);
            non_bottom_states_NewBotSt.clear();
            // Some HitSmall states may also be not-yet-found NewBotSt states,
            // so we have to clear these state counters as well.
            clear_state_counters
                          (bri.potential_non_bottom_states_HitSmall.begin(),
                           bri.potential_non_bottom_states_HitSmall.end(), bi);
            clear(bri.potential_non_bottom_states_HitSmall);
            /* Split off the third subblock (AvoidLrg)                       */ static_assert(2==AvoidLrg);  assert(finished==status[AvoidLrg]);
            if (new_start_bottom_states(AvoidLrg)!=
                                           new_start_bottom_states(AvoidLrg+1))
            {
              move_nonbottom_states_to(
                      non_bottom_states[AvoidLrg],
                      new_end_bottom_states(AvoidLrg)
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  , bri.bottom_size(AvoidLrg)
                                                                                #endif
                                                                             );
              if (bri.start_bottom_states[AvoidLrg]!=
                                             new_start_bottom_states(AvoidLrg))
              {
                multiple_swap_states_in_states_in_block
                                          (bri.start_bottom_states[AvoidLrg],
                                           new_start_bottom_states(AvoidLrg),
                                           bri.bottom_size(AvoidLrg)
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  , bri.start_bottom_states[AvoidLrg],
                                                                                    check_complexity::log_n-
                                                                                                  check_complexity::ilog2(bottom_and_non_bottom_size(AvoidLrg))
                                                                                #endif
                                                                             );
              }
              non_bottom_states[AvoidLrg].clear(); // cannot clear before the above call to bottom_and_non_bottom_size(2)
              create_new_block(new_start_bottom_states(AvoidLrg),
                               new_end_bottom_states(AvoidLrg),
                               new_start_bottom_states(AvoidLrg+1), bi);
              check_incoming_tau_transitions_become_noninert
                                         (NewBotSt_block_index,
                                          new_start_bottom_states(AvoidLrg),
                                          new_start_bottom_states(AvoidLrg+1));
            }                                                                   else {
                                                                                  assert(0==bri.bottom_size(AvoidLrg));
                                                                                  assert(non_bottom_states[AvoidLrg].empty());
                                                                                }
            /* Split off the second subblock (AvoidSml)                      */ static_assert(1==AvoidSml);  assert(finished==status[AvoidSml]);
            if (new_start_bottom_states(AvoidSml)!=
                 new_start_bottom_states(AvoidSml+1))
            {                                                                   assert(0<bri.bottom_size(AvoidSml));
              move_nonbottom_states_to(
                      non_bottom_states[AvoidSml],
                      new_end_bottom_states(AvoidSml)
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  , bri.bottom_size(AvoidSml)
                                                                                #endif
                                                                             );
              if (bri.start_bottom_states[AvoidSml]!=
                                             new_start_bottom_states(AvoidSml))
              {
                multiple_swap_states_in_states_in_block
                                          (bri.start_bottom_states[AvoidSml],
                                           new_start_bottom_states(AvoidSml),
                                           bri.bottom_size(AvoidSml)
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  , bri.start_bottom_states[AvoidSml],
                                                                                    check_complexity::log_n-
                                                                                                  check_complexity::ilog2(bottom_and_non_bottom_size(AvoidSml))
                                                                                #endif
                                                                             );
              }
              non_bottom_states[AvoidSml].clear(); // cannot clear before the above call to bottom_and_non_bottom_size(AvoidLrg)
              create_new_block(new_start_bottom_states(AvoidSml),
                               new_end_bottom_states(AvoidSml),
                               new_start_bottom_states(AvoidSml+1), bi);
              check_incoming_tau_transitions_become_noninert
                                         (NewBotSt_block_index,
                                          new_start_bottom_states(AvoidSml),
                                          new_start_bottom_states(AvoidSml+1));
            }                                                                   else {
                                                                                  assert(0==bri.bottom_size(AvoidSml));
                                                                                  assert(non_bottom_states[AvoidSml].empty());
                                                                                }

            /* Split off the first subblock (ReachAlw)                       */ static_assert(0==ReachAlw);  assert(finished==status[ReachAlw]);
            block_type_lb* ReachAlw_block_index = null_block_lb;
            if (bri.start_bottom_states[ReachAlw]!=
                                           new_start_bottom_states(ReachAlw+1))
            {                                                                   assert(0<bri.bottom_size(ReachAlw));
              move_nonbottom_states_to(non_bottom_states[ReachAlw],
                                          bri.start_bottom_states[ReachAlw+1]
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  , bri.bottom_size(ReachAlw)
                                                                                #endif
                                                                             );
              non_bottom_states[ReachAlw].clear();
              ReachAlw_block_index=create_new_block
                                     (bri.start_bottom_states[ReachAlw],
                                      bri.start_bottom_states[ReachAlw+1],
                                      new_start_bottom_states(ReachAlw+1), bi);
              check_incoming_tau_transitions_become_noninert
                                         (NewBotSt_block_index,
                                          bri.start_bottom_states[ReachAlw],
                                          new_start_bottom_states(ReachAlw+1));
            }                                                                   else {
                                                                                  assert(0==bri.bottom_size(ReachAlw));
                                                                                  assert(non_bottom_states[ReachAlw].empty());
            /* Algorithm 3, Line 3.41–3.42                                   */ }
            NewBotSt_block_index.contains_new_bottom_states=true;               assert(NewBotSt_block_index.start_bottom_states<
                                                                                                                NewBotSt_block_index.sta.rt_non_bottom_states);
            m_blocks_with_new_bottom_states.push_back(&NewBotSt_block_index);
            if (half_orig_bi_size >= number_of_states_in_block(bi))
            {
              // Every new sub-block is small anyway, but in this situation the
              // old block has diminished in size so much that it is a small
              // subset of its former extent.
              bi.is_small_subblock = true;
            }
            return ReachAlw_block_index;
            #undef new_start_bottom_states
            #undef new_end_bottom_states
            #undef new_end_bottom_states_NewBotSt
          }
        } // end of inner coroutine loop for the ReachAlw/AvoidLrg/AvoidSml-states

        // Now do one step for the NewBotSt-states:

        if (incoming_inert_transition_checking==status_NewBotSt)
        {                                                                       assert(current_source_iter_NewBotSt<current_source_iter_end_NewBotSt);
          /* Algorithm 3, Line 3.11 right                                    */ mCRL2complexity(&m_transitions[std::distance(m_aut.get_transitions().begin(),
                                                                                            current_source_iter_NewBotSt)], add_work(check_complexity::
                                                                                                     simple_splitB_R__handle_transition_to_R_state, 1), *this);
          const transition& tr=*current_source_iter_NewBotSt++;                 assert(m_aut.is_tau(m_aut_apply_hidden_label_map(tr.label())));
          state_in_block_pointer_lb const src=m_states.begin()+tr.from();       assert(m_states[tr.to()].block==&bi);
          // Algorithm 3, Line 3.12 right
          if (src.ref_state->block==&bi &&
              !(m_preserve_divergence && tr.from()==tr.to()))
          {
            // Algorithm 3, Line 3.13 right
            if (marked_NewBotSt != src.ref_state->counter)
            {                                                                   assert(!non_bottom_states_NewBotSt.find(src));
              // Algorithm 3, Line 3.8
              if (abort_if_non_bottom_size_too_large_NewBotSt(1))
              {
                // but actually if NewBotSt is already aborted, there is no
                // need to add the state to NewBotSt.  (If the state has
                // block-inert transitions to other subblocks, it will be added
                // to NewBotSt later anyway, but otherwise we have saved the
                // assignment.)
                continue;
              }
              src.ref_state->counter = marked_NewBotSt;
              non_bottom_states_NewBotSt.add_todo(src);
            }                                                                   else  {  assert(non_bottom_states_NewBotSt.find(src));  }
          }
          if (current_source_iter_NewBotSt==current_source_iter_end_NewBotSt ||
              !m_aut.is_tau(m_aut_apply_hidden_label_map
                                      (current_source_iter_NewBotSt->label())))
          {
            status_NewBotSt=state_checking;
          }
        }
        else if (state_checking==status_NewBotSt)
        {
          // Algorithm 3, Line 3.10 right
          if (!non_bottom_states_NewBotSt.todo_is_empty())
          {
            state_in_block_pointer_lb
                               tgt=non_bottom_states_NewBotSt.move_from_todo();
            /* Prepare for the sources of tgt to be added to the subblock    */ mCRL2complexity(tgt.ref_state,
                                                                                     add_work(check_complexity::simple_splitB_R__find_predecessors, 1), *this);
            current_source_iter_NewBotSt=
                                     tgt.ref_state->start_incoming_transitions;
            current_source_iter_end_NewBotSt=
                  std::next(tgt.ref_state)>=m_states.end()
                        ? m_aut.get_transitions().end()
                        : std::next(tgt.ref_state)->start_incoming_transitions;
            if(current_source_iter_NewBotSt<current_source_iter_end_NewBotSt &&
                m_aut.is_tau(m_aut_apply_hidden_label_map
                                      (current_source_iter_NewBotSt->label())))
            {
              status_NewBotSt=incoming_inert_transition_checking;
            }
            continue;
          }
          // Algorithm 3, Line 3.14 right
          if (1>=no_of_finished_searches)
          {
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
            /* Nothing can be done now for the NewBotSt-subblock; we just    */   check_complexity::wait();
            // have to wait for another subblock to give us some initial
            // NewBotSt-state.
                                                                                #endif
            continue;
          }
          // Algorithm 3, Line 3.15 right
          if (finished != status[AvoidLrg] &&
              large_splitter_iter_NewBotSt!=large_splitter_iter_end_NewBotSt &&
              !large_splitter_is_known_to_be_a_strict_BLC_set)
          {                                                                     assert(large_splitter_iter_NewBotSt==bri.large_splitter->start_same_BLC);
            /* Algorithm 3, Line 3.19 right                                  */ assert(large_splitter_iter_end_NewBotSt==bri.large_splitter->end_same_BLC);
            if(&bi!=bi.block_BLC_source->start_BLC_source->ref_state->block ||
               &bi!=bi.block_BLC_source->end_BLC_source[-1].ref_state->block)
            {
              // The BLC set contains other source blocks in addition to bi,
              // so it needs to be simplified.
              make_BLC_simple(bi, false, old_constellation, new_constellation);
//std::cerr << "Now large_splitter=="; if (nullptr == bri.large_splitter) { std::cerr << "nullptr\n"; } else { std::cerr << bri.large_splitter->debug_id(*this) << '\n'; }
              if (nullptr == bri.large_splitter)
              {
                // It turned out that there were not really any transitions
                // from the current block in the large splitter.
                large_splitter_iter_NewBotSt=m_BLC_transitions.data_end();
                large_splitter_iter_end_NewBotSt=m_BLC_transitions.data_end();
              }
              else
              {
                large_splitter_iter_NewBotSt=bri.large_splitter->start_same_BLC;
                large_splitter_iter_end_NewBotSt =
                                              bri.large_splitter->end_same_BLC;
              }
            }
            large_splitter_is_known_to_be_a_strict_BLC_set = true;
          }
          if (finished != status[AvoidLrg] &&
              large_splitter_iter_NewBotSt!=large_splitter_iter_end_NewBotSt)
          {                                                                     assert(finished==status[ReachAlw]);  assert(finished==status[AvoidSml]);
            // Because we have nothing else to do, we handle one transition in
            // the large splitter.

            do
            {
              // Algorithm 3, Line 3.20 right
              const transition&
                      t=m_aut.get_transitions()[*large_splitter_iter_NewBotSt]; mCRL2complexity(&m_transitions[*large_splitter_iter_NewBotSt],
                                                                                          add_work(check_complexity::
                                                                                                   simple_splitB_R__handle_transition_from_R_state, 1), *this);
              ++large_splitter_iter_NewBotSt;
              state_in_block_pointer_lb src = m_states.begin() + t.from();      assert(src.ref_state->block==&bi);
              // Algorithm 3, Line 3.22 right
              if (0==src.ref_state->no_of_outgoing_block_inert_transitions)
              {                                                                 assert(!(bri.start_bottom_states[AvoidLrg]<=src.ref_state->ref_states_in_blocks &&
                                                                                         src.ref_state->ref_states_in_blocks<bri.start_bottom_states[AvoidLrg+1]));
              }
              else
              {
                // Algorithm 3, Line 3.21 right
                if (undefined==src.ref_state->counter ||
                    is_in_marked_range_of(src.ref_state->counter, AvoidLrg))
                {                                                               assert(!non_bottom_states[ReachAlw].find(src));
                  /* The only subblocks that src could go to are AvoidLrg    */ assert(!non_bottom_states[AvoidSml].find(src));
                  /* and NewBotSt.  But because it has a transition in the   */ assert(!non_bottom_states[AvoidLrg].find(src));
                  /* co-splitter, it cannot go to AvoidLrg.                  */ assert(!non_bottom_states_NewBotSt.find(src));
                  // Algorithm 3, Line 3.23 right
                  src.ref_state->counter = marked_NewBotSt;
                  non_bottom_states_NewBotSt.add_todo(src);
                  if (0==no_of_running_searches)
                  {
                    // NewBotSt is the only running search (and AvoidLrg is not
                    // finished, so it must be aborted), so we can as well
                    // continue this loop until we've found all such states.
                    // We also know that NewBotSt cannot become too large.
                    continue;
                  }
                  // We must add state src to NewBotSt even if NewBotSt is
                  // about to be aborted: it may happen that this was exactly
                  // the last transition in the co-splitter, and then the
                  // AvoidLrg-coroutine could add state src erroneously.
                  abort_if_non_bottom_size_too_large_NewBotSt(0);
                  break;
                }                                                               else  {  assert(marked_HitSmall!=src.ref_state->counter);  }
              }
              if (0!=no_of_running_searches)
              {
                break;
              }
            }
            while (                                                             assert(0==no_of_running_searches), assert(aborted==status[AvoidLrg]),
               large_splitter_iter_NewBotSt!=large_splitter_iter_end_NewBotSt);
          }
          else
          {                                                                     // Now check that there were not too many waiting cycles:
                                                                                #ifndef NDEBUG
                                                                                  check_complexity::check_waiting_cycles();
                                                                                  // After this check we are no longer allowed to wait (and we are allowed to
                                                                                  // cancel work).
                                                                                #endif
            // If finished==status[AvoidLrg]:
                // At most one of AvoidSml and ReachAlw is not finished.
                // If AvoidSml is not finished, all states with non-exclusive
                // block-inert transitions to AvoidLrg or ReachAlw have
                // been added to NewBotSt.  Also all states that would
                // be in AvoidLrg except for their transition in the
                // co-splitter have been added to NewBotSt.  The search for
                // AvoidSml-predecessors will not add any further states to
                // NewBotSt.
                // If ReachAlw is not finished, the situation is similar.
                // Therefore, we can finish NewBotSt.
            // If finished!=status[AvoidLrg] &&
            // large_splitter_iter_NewBotSt==large_splitter_iter_end_NewBotSt:
                // Until now, AvoidLrg and NewBotSt were still running, and it
                // was unclear which of the two was smaller.  Now it has turned
                // out that NewBotSt has finished all it can do, so AvoidLrg
                // shall be aborted.
            // Algorithm 3, Line 3.17 right
            status_NewBotSt=finished;                                           assert(3==++no_of_finished_searches);

            // Algorithm 3, Line 3.39
            // Calculate the placement of subblocks, and also clear state
            // counters of the aborted subblock:
            state_in_block_pointer_lb* new_start_bottom_states_plus_one[3];
            state_in_block_pointer_lb* new_end_bottom_states_plus_one[2];
            #define new_start_bottom_states(idx) (assert(1<=(idx)), assert((idx)<=3), new_start_bottom_states_plus_one[(idx)-1])
            #define new_end_bottom_states(idx) (assert(1<=(idx)), assert((idx)<=2), new_end_bottom_states_plus_one[(idx)-1])
            #define new_end_bottom_states_NewBotSt (new_start_bottom_states_plus_one[2])

            const state_index half_orig_bi_size=
                                               number_of_states_in_block(bi)/2;
            new_end_bottom_states_NewBotSt=bi.end_states-
                                             non_bottom_states_NewBotSt.size();

            if (finished == status[AvoidLrg])
            {
              new_end_bottom_states(AvoidLrg)=
                         new_start_bottom_states(AvoidLrg+1)-
                                            non_bottom_states[AvoidLrg].size();
              new_start_bottom_states(AvoidLrg)=
                     new_end_bottom_states(AvoidLrg)-bri.bottom_size(AvoidLrg);
              if (finished==status[AvoidSml])
              {                                                                 assert(finished==status[AvoidSml]);  assert(finished!=status[ReachAlw]);
                new_end_bottom_states(AvoidSml)=
                             new_start_bottom_states(AvoidSml+1)-
                                            non_bottom_states[AvoidSml].size();
                new_start_bottom_states(AvoidSml)=
                     new_end_bottom_states(AvoidSml)-bri.bottom_size(AvoidSml);
                // clear the state counters of the aborted subblock:
                non_bottom_states[ReachAlw].clear();
                clear_state_counters
                         (bri.potential_non_bottom_states[ReachAlw].begin(),
                          bri.potential_non_bottom_states[ReachAlw].end(), bi);
                clear(bri.potential_non_bottom_states[ReachAlw]);
                // Some HitSmall states may still linger around in the aborted
                // subblock.  So we also have to clear these state counters.
                if (nullptr != bri.large_splitter)
                {
                  clear_state_counters
                          (bri.potential_non_bottom_states_HitSmall.begin(),
                           bri.potential_non_bottom_states_HitSmall.end(), bi);
                }                                                               else  {  assert(bri.potential_non_bottom_states_HitSmall.empty());  }
              }
              else
              {                                                                 assert(finished==status[ReachAlw]);
                new_start_bottom_states(AvoidSml)=
                             bri.start_bottom_states[ReachAlw+1]+
                                            non_bottom_states[ReachAlw].size();
                new_end_bottom_states(AvoidSml)=
                   new_start_bottom_states(AvoidSml)+bri.bottom_size(AvoidSml);
                // clear the state counters of the aborted subblock:
                non_bottom_states[AvoidSml].clear();
                clear_state_counters
                         (bri.potential_non_bottom_states[AvoidSml].begin(),
                          bri.potential_non_bottom_states[AvoidSml].end(), bi);
                clear(bri.potential_non_bottom_states[AvoidSml]);
                // All HitSmall states must have been captured by another
                // subblock.  So we can just delete them.
              }
            }
            else
            {                                                                   assert(finished==status[ReachAlw]);
              new_start_bottom_states(AvoidSml)=
                                 bri.start_bottom_states[ReachAlw+1]+
                                            non_bottom_states[ReachAlw].size(); assert(finished==status[AvoidSml]);
              new_end_bottom_states(AvoidSml)=
                   new_start_bottom_states(AvoidSml)+bri.bottom_size(AvoidSml);
              new_start_bottom_states(AvoidLrg)=
                              new_end_bottom_states(AvoidSml)+
                                            non_bottom_states[AvoidSml].size();
              new_end_bottom_states(AvoidLrg)=
                   new_start_bottom_states(AvoidLrg)+bri.bottom_size(AvoidLrg);
              // clear the state counters of the aborted subblock:
              non_bottom_states[AvoidLrg].clear();
              clear_state_counters
                         (bri.potential_non_bottom_states[AvoidLrg].begin(),
                          bri.potential_non_bottom_states[AvoidLrg].end(), bi);
              clear(bri.potential_non_bottom_states[AvoidLrg]);
              // Some HitSmall states may still linger around.
              clear_state_counters
                          (bri.potential_non_bottom_states_HitSmall.begin(),
                           bri.potential_non_bottom_states_HitSmall.end(), bi);
            }
            clear(bri.potential_non_bottom_states_HitSmall);
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  // Finish the accounting.
                                                                                  // (We have already called `check_complexity::check_waiting_cycles()`, so we
                                                                                  // are no longer allowed to wait, and we are allowed to cancel work.)
                                                                                  // Cancel work in the whole block (actually only work in the aborted subblock
                                                                                  // will be cancelled, but we go through the whole block because the states
                                                                                  // have not yet been positioned correctly; also, most likely not all its
                                                                                  // non-bottom states will be in `non_bottom_states[...]`).
                                                                                  {
                                                                                    state_index max_NcludeCo_size=std::distance
                                                                                                               (new_end_bottom_states_NewBotSt, bi.end_states);
                                                                                    max_NcludeCo_size=std::max<state_index>(max_NcludeCo_size, std::distance(
                                                                                      bri.start_bottom_states[ReachAlw], new_start_bottom_states(ReachAlw+1)));
                                                                                    max_NcludeCo_size=std::max<state_index>(max_NcludeCo_size, std::distance(
                                                                                       new_start_bottom_states(AvoidSml),new_start_bottom_states(AvoidSml+1)));
                                                                                    const unsigned char max_NcludeCo_B = 0==max_NcludeCo_size ? 0
                                                                                          : check_complexity::log_n-check_complexity::ilog2(max_NcludeCo_size);
                                                                                    const state_in_block_pointer_lb* s=bi.start_bottom_states;
                                                                                    do {
                                                                                      mCRL2complexity(s->ref_state, cancel_work
                                                                                                (check_complexity::simple_splitB_U__find_predecessors), *this);
                                                                                      // incoming tau-transitions of s
                                                                                      const std::vector<transition>::const_iterator in_ti_end=
                                                                                        std::next(s->ref_state)>=m_states.end() ? m_aut.get_transitions().end()
                                                                                                         : std::next(s->ref_state)->start_incoming_transitions;
                                                                                      for (std::vector<transition>::const_iterator
                                                                                              ti=s->ref_state->start_incoming_transitions; ti!=in_ti_end; ++ti)
                                                                                      {
                                                                                        if(!m_aut.is_tau(m_aut_apply_hidden_label_map(ti->label()))) { break; }
                                                                                        mCRL2complexity(&m_transitions[std::distance(m_aut.get_transitions().
                                                                                                cbegin(), ti)], cancel_work(check_complexity::
                                                                                                        simple_splitB_U__handle_transition_to_U_state), *this);
                                                                                      }
                                                                                      if (finished!=status[AvoidLrg]) {
                                                                                        // outgoing transitions of s
                                                                                        const outgoing_transitions_const_it_lb out_ti_end=
                                                                                         std::next(s->ref_state)>=m_states.end() ? m_outgoing_transitions.end()
                                                                                                         : std::next(s->ref_state)->start_outgoing_transitions;
                                                                                        for (outgoing_transitions_const_it_lb
                                                                                             ti=s->ref_state->start_outgoing_transitions; ti!=out_ti_end; ++ti)
                                                                                        {
                                                                                          mCRL2complexity(&m_transitions[*ti->ref_BLC_transitions],
                                                                                            cancel_work(check_complexity::
                                                                                            simple_splitB_U__handle_transition_from_potential_U_state), *this);
                                                                                          // We should also finalise the co-splitter transitions handled by
                                                                                          // NewBotSt (which may exist even if NewBotSt is empty):
                                                                                          mCRL2complexity(&m_transitions[*ti->ref_BLC_transitions],
                                                                                              finalise_work(check_complexity::
                                                                                                            simple_splitB_R__handle_transition_from_R_state,
                                                                                                            check_complexity::
                                                                                                            simple_splitB__handle_transition_from_R_or_U_state,
                                                                                                                                       max_NcludeCo_B), *this);
                                                                                        }
                                                                                      }
                                                                                    } while (++s!=bi.end_states);
                                                                                  }
                                                                                #endif
            // split off NewBotSt
            // This can be done only after the aborted subblock has cleared
            // its state counters.  But it should be done before the other
            /* splits, so it is easy to detect which transitions are no      */ assert((state_index) std::distance(new_end_bottom_states_NewBotSt,
            /* longer block-inert.                                           */                             bi.end_states)==non_bottom_states_NewBotSt.size());
            if (new_end_bottom_states_NewBotSt!=bi.end_states)
            {                                                                   assert(!non_bottom_states_NewBotSt.empty());
              /* As NewBotSt is not empty, a trivial constellation will      */ assert(bi.start_bottom_states<new_end_bottom_states_NewBotSt);
              // become non-trivial.  (The condition in if() needs to be
              // checked before the subblock for NewBotSt is created.)
              constellation_type_lb& constellation=*bi.constellation;
              if (constellation.start_const_states->ref_state->block==
                   std::prev(constellation.end_const_states)->ref_state->block)
              {                                                                 assert(std::find(m_non_trivial_constellations.begin(),
                /* This constellation was trivial, as it will be split add it*/                  m_non_trivial_constellations.end(),
                /* to the non-trivial constellations.                        */                  &constellation)==m_non_trivial_constellations.end());
                m_non_trivial_constellations.emplace_back(&constellation);
              }

              move_nonbottom_states_to(non_bottom_states_NewBotSt,
                                               new_end_bottom_states_NewBotSt
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  , 0
                                                                                #endif
                                                                             );
              non_bottom_states_NewBotSt.clear();
              block_type_lb& NewBotSt_block_index=
                 *create_new_block(new_end_bottom_states_NewBotSt,
                                   new_end_bottom_states_NewBotSt,
                                   bi.end_states, bi);
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  // Finalise the work in NewBotSt.  This should be done after calling `
                                                                                  // check_complexity::check_waiting_cycles()` so NewBotSt cannot make its own
                                                                                  // waiting time appear small.
                                                                                  assert(new_end_bottom_states_NewBotSt<bi.end_states);
                                                                                  const unsigned char max_new_B=check_complexity::log_n-check_complexity::ilog2
                                                                                                (std::distance(new_end_bottom_states_NewBotSt, bi.end_states));
                                                                                  const state_in_block_pointer_lb* s=new_end_bottom_states_NewBotSt;
                                                                                  do {
                                                                                    mCRL2complexity(s->ref_state, finalise_work(check_complexity::
                                                                                          simple_splitB_R__find_predecessors, check_complexity::
                                                                                          simple_splitB__find_predecessors_of_R_or_U_state, max_new_B), *this);
                                                                                    // incoming tau-transitions of s
                                                                                    const std::vector<transition>::iterator in_ti_end=
                                                                                        std::next(s->ref_state)>=m_states.end() ? m_aut.get_transitions().end()
                                                                                                         : std::next(s->ref_state)->start_incoming_transitions;
                                                                                    for (std::vector<transition>::iterator
                                                                                              ti=s->ref_state->start_incoming_transitions; ti!=in_ti_end; ++ti)
                                                                                    {
                                                                                      if (!m_aut.is_tau(m_aut_apply_hidden_label_map(ti->label()))) { break; }
                                                                                      mCRL2complexity(&m_transitions[std::distance(m_aut.get_transitions().
                                                                                                begin(), ti)], finalise_work(check_complexity::
                                                                                          simple_splitB_R__handle_transition_to_R_state, check_complexity::
                                                                                          simple_splitB__handle_transition_to_R_or_U_state, max_new_B), *this);
                                                                                    }
                                                                                    // outgoing transitions of s -- already done above if necessary
                                                                                    ++s;
                                                                                  } while (s!=bi.end_states);
                                                                                  // Reset the work balance counters:
                                                                                  check_complexity::check_temporary_work();
                                                                                #endif
              // Algorithm 3, Line 3.40
              // check transitions that have become non-block-inert:
              state_in_block_pointer_lb* nst_it=new_end_bottom_states_NewBotSt; assert(nst_it!=bi.end_states);
              do
              {
                outgoing_transitions_const_it_lb const
                  out_it_end = std::next(nst_it->ref_state)>=m_states.end()
                    ? m_outgoing_transitions.end()
                    : std::next(nst_it->ref_state)->start_outgoing_transitions;
                outgoing_transitions_it_lb out_it=
                                 nst_it->ref_state->start_outgoing_transitions; assert(out_it!=out_it_end);
                const transition* tr=&m_aut.get_transitions()
                                                [*out_it->ref_BLC_transitions]; assert(0<nst_it->ref_state->no_of_outgoing_block_inert_transitions);
                do
                {                                                               assert(m_states.begin()+tr->from()==nst_it->ref_state);
                                                                                assert(m_aut.is_tau(m_aut_apply_hidden_label_map(tr->label())));
                  if (m_states[tr->to()].block==&bi)
                  {                                                             assert(is_inert_during_init(*tr));
                    /* This is a transition that has become non-block-inert. */ assert(bi.start_bottom_states<=m_states[tr->to()].ref_states_in_blocks);
                    /* (However, it is still constellation-inert.)           */
                    /* make_transition_non_inert(*tr)                        */ assert(m_states[tr->to()].ref_states_in_blocks<new_end_bottom_states_NewBotSt);
                    /* < would just execute the decrement "--" below:        */ assert(0<nst_it->ref_state->no_of_outgoing_block_inert_transitions);
                    if (0== --nst_it->ref_state->
                                        no_of_outgoing_block_inert_transitions)
                    {
                      // The state at nst_it has become a bottom_state.
                      change_non_bottom_state_to_bottom_state
                                                           (nst_it->ref_state);
                      break;
                    }
                  }                                                             else {
                                                                                  assert(new_end_bottom_states_NewBotSt<=
                                                                                                                     m_states[tr->to()].ref_states_in_blocks ||
                                                                                        m_states[tr->to()].ref_states_in_blocks<bri.start_bottom_states[ReachAlw]);
                                                                                }
                  ++out_it;
                }
                while (out_it!=out_it_end &&
                  (tr=&m_aut.get_transitions()[*out_it->ref_BLC_transitions],
                     m_aut.is_tau(m_aut_apply_hidden_label_map(tr->label()))));
                ++nst_it;
              }
              while (nst_it!=bi.end_states);                                    assert(NewBotSt_block_index.start_bottom_states<
              /* Algorithm 3, Line 3.42                                      */                                 NewBotSt_block_index.sta.rt_non_bottom_states);
              NewBotSt_block_index.contains_new_bottom_states=true;
              m_blocks_with_new_bottom_states.push_back(&NewBotSt_block_index);
            }
            else
            {
                                                                                #ifndef NDEBUG
                                                                                  // Reset the work balance counters:
                                                                                  check_complexity::check_temporary_work();
                                                                                #endif
                                                                                assert(bri.start_bottom_states[AvoidSml]<bri.start_bottom_states[AvoidLrg+1]);
              if (bri.start_bottom_states[ReachAlw] ==
                                         bri.start_bottom_states[ReachAlw+1] &&
                  (bri.start_bottom_states[AvoidSml] ==
                                       bri.start_bottom_states[AvoidSml+1] ||
                    bri.start_bottom_states[AvoidLrg] ==
                                          bri.start_bottom_states[AvoidLrg+1]))
              {
                // the split is actually trivial.
                // the potential non-bottom state counters of the aborted
                // subblock (i.e. of the whole block) have already been reset
                // above.
                // we only need to clear the data structures that still
                // remain... and the easiest way to run all the checks is to
                // still go through the checks below.
              }
              else
              {
                constellation_type_lb& constellation = *bi.constellation;       assert(non_bottom_states_NewBotSt.empty());
                if (constellation.start_const_states->ref_state->block==
                   std::prev(constellation.end_const_states)->ref_state->block)
                {                                                               assert(std::find(m_non_trivial_constellations.begin(),
                  /* This constellation was trivial, as it will be split add */                  m_non_trivial_constellations.end(),
                  /* it to the non-trivial constellations.                   */                  &constellation)==m_non_trivial_constellations.end());
                  m_non_trivial_constellations.emplace_back(&constellation);    assert((bri.start_bottom_states[ReachAlw]!=new_start_bottom_states(ReachAlw+1))+
                                                                                       (new_start_bottom_states(AvoidSml)!=
                                                                                                                       new_start_bottom_states(AvoidSml+1))+
                                                                                       (new_start_bottom_states(AvoidLrg)!=
                                                                                                                       new_start_bottom_states(AvoidLrg+1))>1);
                }
              }
            }                                                                   assert(finished!=status[AvoidLrg] || static_cast<state_index>(std::distance
            /* Algorithm 3, Line 3.39                                        */     (new_start_bottom_states(AvoidLrg), new_start_bottom_states(AvoidLrg+1)))==
            /* Split off the third subblock (AvoidLrg)                       */                                          bottom_and_non_bottom_size(AvoidLrg));
            if (new_start_bottom_states(AvoidLrg) !=
                                           new_start_bottom_states(AvoidLrg+1))
            {                                                                   assert(0!=bri.bottom_size(AvoidLrg));
              if (bri.start_bottom_states[AvoidLrg]!=
                                             new_start_bottom_states(AvoidLrg))
              {
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  const state_in_block_pointer_lb* acct_iter;
                                                                                  state_index acct_B_size;
                                                                                  if (finished==status[AvoidLrg]) {
                                                                                    acct_iter=bri.start_bottom_states[AvoidLrg];
                                                                                    acct_B_size=bottom_and_non_bottom_size(AvoidLrg);
                                                                                  } else {
                                                                                    // If AvoidLrg is aborted, the work can be assigned to the non-bottom
                                                                                    // states of ReachAlw and AvoidSml.
                                                                                    assert(non_bottom_states[AvoidLrg].empty());
                                                                                    assert(finished==status[ReachAlw]);  assert(finished==status[AvoidSml]);
                                                                                    state_index count=std::min<state_index>(bri.bottom_size(AvoidLrg),
                                                                                                              std::distance(bri.start_bottom_states[AvoidLrg],
                                                                                                                           new_start_bottom_states(AvoidLrg)));
                                                                                    if (non_bottom_states[AvoidSml].size()>=count) {
                                                                                      acct_iter=non_bottom_states[AvoidSml].data();
                                                                                      acct_B_size=bottom_and_non_bottom_size(AvoidSml);
                                                                                    } else if (non_bottom_states[ReachAlw].size()>=count) {
                                                                                      acct_iter=non_bottom_states[ReachAlw].data();
                                                                                      acct_B_size=bottom_and_non_bottom_size(ReachAlw);
                                                                                    } else {
                                                                                      assert(count<=non_bottom_states[AvoidSml].size()+
                                                                                                    non_bottom_states[ReachAlw].size());
                                                                                      // As we are not going to use  `non_bottom_states[AvoidLrg]` for anything
                                                                                      // else, we just replace its content by the relevant states.
                                                                                      non_bottom_states[AvoidLrg]=non_bottom_states[AvoidSml];
                                                                                      non_bottom_states[AvoidLrg].add_todo(non_bottom_states[ReachAlw].begin(),
                                                                                                  non_bottom_states[ReachAlw].begin()
                                                                                                                  +(count-non_bottom_states[AvoidLrg].size()));
                                                                                      acct_iter=non_bottom_states[AvoidLrg].data();
                                                                                      acct_B_size=std::max(bottom_and_non_bottom_size(AvoidSml),
                                                                                                           bottom_and_non_bottom_size(ReachAlw));
                                                                                    }
                                                                                  }  assert(0<acct_B_size);
                                                                                #endif
                multiple_swap_states_in_states_in_block
                                          (bri.start_bottom_states[AvoidLrg],
                                           new_start_bottom_states(AvoidLrg),
                                           bri.bottom_size(AvoidLrg)
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  , acct_iter, check_complexity::log_n-check_complexity::ilog2(acct_B_size),
                                                                                    finished==status[AvoidLrg]
                                                                                    ?check_complexity::multiple_swap_states_in_block__swap_state_in_small_block
                                                                                    :check_complexity::
                                                                                               multiple_swap_states_in_block__account_for_swap_in_aborted_block
                                                                                #endif
                                                                             );
              }

              if (finished==status[AvoidLrg])
              {                                                                 assert(bri.potential_non_bottom_states[AvoidLrg].empty());
                move_nonbottom_states_to(non_bottom_states[AvoidLrg],
                                              new_end_bottom_states(AvoidLrg)
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  , bri.bottom_size(AvoidLrg)
                                                                                #endif
                                                                             );
                non_bottom_states[AvoidLrg].clear();
                create_new_block(new_start_bottom_states(AvoidLrg),
                                 new_end_bottom_states(AvoidLrg),
                                 new_start_bottom_states(AvoidLrg+1), bi);
              }
              else
              {
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  // delete what we've stored in non_bottom_states[AvoidLrg] just for
                                                                                  // accounting
                                                                                  non_bottom_states[AvoidLrg].clear();
                                                                                #endif
                bi.start_bottom_states = new_start_bottom_states(AvoidLrg);
                bi.sta.rt_non_bottom_states = new_end_bottom_states(AvoidLrg);  assert(bi.start_bottom_states<bi.sta.rt_non_bottom_states);
                bi.end_states = new_start_bottom_states(AvoidLrg+1);            assert(bi.sta.rt_non_bottom_states<=bi.end_states);
              }
            }                                                                   else {
                                                                                  assert(new_start_bottom_states(AvoidLrg)==
                                                                                                                          new_start_bottom_states(AvoidLrg+1));
                                                                                  assert(0==bri.bottom_size(AvoidLrg));assert(non_bottom_states[AvoidLrg].empty());
                                                                                  assert(finished==status[AvoidLrg]);
                                                                                }
            /* Split off the second subblock (AvoidSml)                      */ assert(finished!=status[AvoidSml] || static_cast<state_index>(std::distance
                                                                                    (new_start_bottom_states(AvoidSml), new_start_bottom_states(AvoidSml+1)))==
                                                                                                                         bottom_and_non_bottom_size(AvoidSml));
            if (new_start_bottom_states(AvoidSml)!=
                                           new_start_bottom_states(AvoidSml+1))
            {                                                                   assert(0!=bri.bottom_size(AvoidSml));
              // If AvoidSml is aborted, then swapping these bottom states can
              // be accounted for by the non-bottom states of ReachAlw.
              // The function will not execute more swaps than their size.
              if (bri.start_bottom_states[AvoidSml]!=
                                             new_start_bottom_states(AvoidSml))
              {
                multiple_swap_states_in_states_in_block
                                          (bri.start_bottom_states[AvoidSml],
                                           new_start_bottom_states(AvoidSml),
                                           bri.bottom_size(AvoidSml)
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  , finished==status[AvoidSml] ? bri.start_bottom_states[AvoidSml]
                                                                                                               : non_bottom_states[ReachAlw].data(),
                                                                                    check_complexity::log_n-check_complexity::ilog2
                                                                                        (finished==status[AvoidSml] ? bottom_and_non_bottom_size(AvoidSml)
                                                                                                                    : bottom_and_non_bottom_size(ReachAlw)),
                                                                                    finished==status[AvoidSml]
                                                                                    ?check_complexity::multiple_swap_states_in_block__swap_state_in_small_block
                                                                                    :check_complexity::
                                                                                               multiple_swap_states_in_block__account_for_swap_in_aborted_block
                                                                                #endif
                                                                             );
              }
              if (finished==status[AvoidSml])
              {                                                                 assert(bri.potential_non_bottom_states[AvoidSml].empty());
                move_nonbottom_states_to(non_bottom_states[AvoidSml],
                                              new_end_bottom_states(AvoidSml)
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  , bri.bottom_size(AvoidSml)
                                                                                #endif
                                                                             );
                non_bottom_states[AvoidSml].clear();
                create_new_block(new_start_bottom_states(AvoidSml),
                                 new_end_bottom_states(AvoidSml),
                                 new_start_bottom_states(AvoidSml+1), bi);
              }
              else
              {
                bi.start_bottom_states = new_start_bottom_states(AvoidSml);
                bi.sta.rt_non_bottom_states = new_end_bottom_states(AvoidSml);  assert(bi.start_bottom_states<bi.sta.rt_non_bottom_states);
                bi.end_states = new_start_bottom_states(AvoidSml+1);            assert(bi.sta.rt_non_bottom_states<=bi.end_states);
              }
            }                                                                   else {
                                                                                  assert(new_start_bottom_states(AvoidSml)==
                                                                                                                          new_start_bottom_states(AvoidSml+1));
                                                                                  assert(0==bri.bottom_size(AvoidSml));
                                                                                  assert(non_bottom_states[AvoidSml].empty());
                                                                                  assert(finished==status[AvoidSml]);
                                                                                }
            /* Split off the first subblock (ReachAlw)                       */ assert(finished!=status[ReachAlw] || static_cast<state_index>(std::distance
                                                                                        (bri.start_bottom_states[ReachAlw], new_start_bottom_states(ReachAlw+1)))==
                                                                                                                         bottom_and_non_bottom_size(ReachAlw));
            block_type_lb* ReachAlw_block_index = null_block_lb;
            if (bri.start_bottom_states[ReachAlw]!=
                                           new_start_bottom_states(ReachAlw+1))
            {                                                                   assert(0<bri.bottom_size(ReachAlw));
              if (finished==status[ReachAlw])
              {                                                                 assert(bri.potential_non_bottom_states[ReachAlw].empty());
                move_nonbottom_states_to(non_bottom_states[ReachAlw],
                                         bri.start_bottom_states[ReachAlw+1]
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  , bri.bottom_size(ReachAlw)
                                                                                #endif
                                                                             );
                non_bottom_states[ReachAlw].clear();
                ReachAlw_block_index=create_new_block
                                     (bri.start_bottom_states[ReachAlw],
                                      bri.start_bottom_states[ReachAlw+1],
                                      new_start_bottom_states(ReachAlw+1), bi);
              }
              else
              {                                                                 assert(bi.start_bottom_states==bri.start_bottom_states[ReachAlw]);
                bi.sta.rt_non_bottom_states =
                                           bri.start_bottom_states[ReachAlw+1]; assert(bi.start_bottom_states<bi.sta.rt_non_bottom_states);
                bi.end_states = new_start_bottom_states(ReachAlw+1);            assert(bi.sta.rt_non_bottom_states<=bi.end_states);
                ReachAlw_block_index=&bi;
              }
            }                                                                   else {
                                                                                  assert(0==bri.bottom_size(ReachAlw));assert(non_bottom_states[ReachAlw].empty());
            /* Algorithm 3, Line 3.43                                        */ }
            if (half_orig_bi_size >= number_of_states_in_block(bi))
            {
              // Every new sub-block is small anyway, but in this situation the
              // old block has diminished in size so much that it is a small
              // subset of its former extent.
              bi.is_small_subblock = true;
            }
            return ReachAlw_block_index; // leave the function completely, as we have finished.
            #undef new_start_bottom_states
            #undef new_end_bottom_states
            #undef new_end_bottom_states_NewBotSt
          }
        }                                                                       else {
                                                                                  assert(aborted==status_NewBotSt);
                                                                                }
      } // end of outer coroutine loop for ReachAlw/AvoidSml/AvoidLrg and NewBotSt together

      #undef abort_if_bottom_size_too_large
      #undef abort_if_non_bottom_size_too_large_NewBotSt
      #undef abort_if_size_too_large
      #undef bottom_and_non_bottom_size
      #undef non_bottom_states_NewBotSt
    }

//================================================= Create initial partition ========================================================
    transition_index accumulate_entries(
                              std::vector<transition_index>& action_counter,
                              const std::vector<label_index>& todo_stack) const
    {
      transition_index sum=0;
      for(label_index index: todo_stack)
      {                                                                         // The work in this loop is attributed to the transitions with label `index`
        transition_index n=sum;
        sum=sum+action_counter[index];
        action_counter[index]=n;
      }
      return sum;
    }

/*    /// \brief create one BLC set for the block starting at `pos`
    /// \details The BLC set is created, inserted into the list
    /// `block.to_constellation` of the block, and the pointers from
    /// transitions to it are adapted.  The function also adapts the
    /// `ref_BLC_transitions` pointer of the transitions in the BLC set.
    void order_BLC_transitions_single_BLC_set(
                                        state_in_block_pointer_lb* const pos,
                                              BLC_list_iterator start_same_BLC,
                                        const BLC_list_iterator end_same_BLC)
    {                                                                           assert(start_same_BLC<end_same_BLC);
      block_type_lb* const bi=pos->ref_state->block;                            assert(pos==bi->start_bottom_states);
      simple_list<BLC_indicators_lb>::iterator blc=
                bi->block_BLC_source->block_to_constellation.
                              emplace_back(start_same_BLC, end_same_BLC, true);
      do
      {                                                                         assert(bi==m_states[m_aut.get_transitions()[*start_same_BLC].from()].block);
        m_transitions[*start_same_BLC].transitions_per_block_to_constellation=
                                                                           blc; mCRL2complexity(&m_transitions[*start_same_BLC], add_work(check_complexity::
                                                                                      order_BLC_transitions__sort_transition, check_complexity::log_n), *this);
        m_transitions[*start_same_BLC].ref_outgoing_transitions->
                                       ref.convert_to_iterator(start_same_BLC);
      }
      while (++start_same_BLC<end_same_BLC);
    }

    /// \brief order `m_BLC_transition` entries according to source block
    /// \param start_same_BLC  first transition to be handled
    /// \param end_same_BLC    iterator past the last transition to be handled
    /// \param min_block       lower bound to the block `start_bottom_states` that can be expected
    /// \param max_block       upper bound to the block `start_bottom_states` that can be expected
    /// \details This function assumes that all transitions in the range
    /// [`start_same_BLC`, `end_same_BLC`) have the same label and the same
    /// target constellation.  They have source blocks whose field
    /// `start_bottom_states` is in the range
    /// [`min_block`, `max_block`].  It groups these transitions according
    /// to their source blocks and inserts the corresponding
    /// `simple_list<BLC_indicators_lb>` entries in the source blocks.  The
    /// algorithm used is similar to quicksort, but the pivot value is
    /// determined by numeric calculations instead of selection from the data.
    ///
    /// The function is intended to be used during initialisation, if one does
    /// not use `m_BLC_transitions` during the first refinements.
    void order_BLC_transitions(const BLC_list_iterator start_same_BLC,
                      const BLC_list_iterator end_same_BLC,
                      state_in_block_pointer_lb* min_block,
                      state_in_block_pointer_lb* max_block)
    {                                                                           assert(start_same_BLC<end_same_BLC);
                                                                                assert(min_block->ref_state->block->start_bottom_states==min_block);
                                                                                assert(max_block->ref_state->block->start_bottom_states==max_block);
      if (min_block==max_block)
      {
        order_BLC_transitions_single_BLC_set(min_block,
                                                 start_same_BLC, end_same_BLC);
        return;
      }                                                                         else  {  assert(min_block<max_block);  }
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  const unsigned char max_sort=check_complexity::log_n-
                                                                                                                check_complexity::ilog2(max_block-min_block+1);
                                                                                #endif
      state_in_block_pointer_lb* pivot=min_block+(max_block-min_block+1)/2;
      pivot=pivot->ref_state->block->start_bottom_states; // round down
      state_in_block_pointer_lb* min_below_pivot=pivot;
      state_in_block_pointer_lb* max_above_pivot=pivot;
      #define max_below_pivot min_block
      #define min_above_pivot max_block
      // move transitions with source_block==pivot to the beginning,
      // transitions with source_block<pivot to the middle,
      // transitions with source_block>pivot to the end
      // (similar to quicksort with equal keys)
      BLC_list_iterator end_equal_to_pivot=start_same_BLC;
      BLC_list_iterator end_smaller_than_pivot=start_same_BLC;
      BLC_list_iterator begin_larger_than_pivot=end_same_BLC;
      for (;;)
      {
        for (;;)
        {                                                                       assert(end_smaller_than_pivot<begin_larger_than_pivot);
                                                                                #ifndef NDEBUG
                                                                                  { const state_in_block_pointer_lb* sb;
                                                                                    BLC_list_const_iterator it=start_same_BLC;
                                                                                    assert(it<=end_equal_to_pivot);
                                                                                    for (; it<end_equal_to_pivot; ++it) {
                                                                                      assert(m_states[m_aut.get_transitions()[*it].from()].block->
                                                                                                                                   start_bottom_states==pivot);
                                                                                    }
                                                                                    assert(it<=end_smaller_than_pivot);
                                                                                    for (; it<end_smaller_than_pivot; ++it) {
                                                                                      assert(max_below_pivot<pivot);
                                                                                      sb=m_states[m_aut.get_transitions()[*it].from()].block->
                                                                                                                                           start_bottom_states;
                                                                                      assert(sb>=min_below_pivot);  assert(sb<=max_below_pivot);
                                                                                    }
                                                                                    assert(it<begin_larger_than_pivot);
                                                                                    for (it=begin_larger_than_pivot; it<end_same_BLC; ++it) {
                                                                                      assert(pivot<min_above_pivot);
                                                                                      sb=m_states[m_aut.get_transitions()[*it].from()].block->
                                                                                                                                           start_bottom_states;
                                                                                      assert(sb>=min_above_pivot);  assert(sb<=max_above_pivot);
                                                                                    }
                                                                                  }
                                                                                #endif
                                                                                mCRL2complexity(&m_transitions[*end_smaller_than_pivot], add_work(
                                                                                   check_complexity::order_BLC_transitions__sort_transition, max_sort), *this);
          state_in_block_pointer_lb* const source_block=
                m_states[m_aut.get_transitions()
                  [*end_smaller_than_pivot].from()].block->start_bottom_states;
          if (source_block==pivot)
          {
            std::swap(*end_equal_to_pivot++, *end_smaller_than_pivot);
          }
          else if (source_block>pivot)
          {
            if (source_block<min_above_pivot)
            {
              min_above_pivot=source_block;
            }
            if (source_block>max_above_pivot)
            {
              max_above_pivot=source_block;
            }
            break;
          }
          else
          {
            if (source_block<min_below_pivot)
            {
              min_below_pivot=source_block;
            }
            if (source_block>max_below_pivot)
            {
              max_below_pivot=source_block;
            }
          }
          ++end_smaller_than_pivot;
          if (end_smaller_than_pivot>=begin_larger_than_pivot)
          {
            goto break_two_loops;
          }
        }
        // Now *end_smaller_than_pivot contains an element with
        // source_block > pivot
        for (;;)
        {                                                                       assert(end_smaller_than_pivot<begin_larger_than_pivot);
                                                                                #ifndef NDEBUG
                                                                                  { const state_in_block_pointer_lb* sb;
                                                                                    BLC_list_const_iterator it=start_same_BLC;
                                                                                    assert(it<=end_equal_to_pivot);
                                                                                    for (; it<end_equal_to_pivot; ++it) {
                                                                                      assert(m_states[m_aut.get_transitions()[*it].from()].block->
                                                                                                                                   start_bottom_states==pivot);
                                                                                    }
                                                                                    assert(it<=end_smaller_than_pivot);
                                                                                    for (; it<end_smaller_than_pivot; ++it) {
                                                                                      assert(max_below_pivot<pivot);
                                                                                      sb=m_states[m_aut.get_transitions()[*it].from()].block->
                                                                                                                                           start_bottom_states;
                                                                                      assert(sb>=min_below_pivot);  assert(sb<=max_below_pivot);
                                                                                    }
                                                                                    assert(it<begin_larger_than_pivot);  assert(pivot<min_above_pivot);
                                                                                    sb=m_states[m_aut.get_transitions()[*it].from()].
                                                                                                                                    block->start_bottom_states;
                                                                                    assert(sb>=min_above_pivot);  assert(sb<=max_above_pivot);
                                                                                    for (it=begin_larger_than_pivot; it<end_same_BLC; ++it) {
                                                                                      sb=m_states[m_aut.get_transitions()[*it].from()].block->
                                                                                                                                           start_bottom_states;
                                                                                      assert(sb>=min_above_pivot);  assert(sb<=max_above_pivot);
                                                                                    }
                                                                                  }
                                                                                #endif
          --begin_larger_than_pivot;
          if (end_smaller_than_pivot>=begin_larger_than_pivot)
          {
            goto break_two_loops;
          }                                                                     mCRL2complexity(&m_transitions[*begin_larger_than_pivot], add_work(
                                                                                   check_complexity::order_BLC_transitions__sort_transition, max_sort), *this);
          state_in_block_pointer_lb* const source_block=
              m_states[m_aut.get_transitions()
                 [*begin_larger_than_pivot].from()].block->start_bottom_states;
          if (source_block==pivot)
          {                                                                     assert(end_smaller_than_pivot<begin_larger_than_pivot);
            transition_index temp=*begin_larger_than_pivot;                     assert(end_equal_to_pivot<=end_smaller_than_pivot);
            *begin_larger_than_pivot=*end_smaller_than_pivot;
            *end_smaller_than_pivot=*end_equal_to_pivot;
            *end_equal_to_pivot=temp;
            ++end_equal_to_pivot;
            ++end_smaller_than_pivot;
            if (end_smaller_than_pivot>=begin_larger_than_pivot)
            {
              goto break_two_loops;
            }
            break;
          }
          if (source_block<pivot)
          {
            if (source_block<min_below_pivot)
            {
              min_below_pivot=source_block;
            }
            if (source_block>max_below_pivot)
            {
              max_below_pivot=source_block;
            }
            std::swap(*end_smaller_than_pivot, *begin_larger_than_pivot);
            ++end_smaller_than_pivot;
            if (end_smaller_than_pivot>=begin_larger_than_pivot)
            {
              goto break_two_loops;
            }
            break;
          }                                                                     assert(min_above_pivot<=max_above_pivot);
          if (source_block<min_above_pivot)
          {
            min_above_pivot=source_block;
          }
          else if (source_block>max_above_pivot)
          {
            max_above_pivot=source_block;
          }
        }
      }
      break_two_loops: ;                                                        assert(end_smaller_than_pivot==begin_larger_than_pivot);
                                                                                #ifndef NDEBUG
                                                                                  { const state_in_block_pointer_lb* sb;
                                                                                    BLC_list_const_iterator it=start_same_BLC;
                                                                                    assert(it<=end_equal_to_pivot);
                                                                                    for (; it<end_equal_to_pivot; ++it) {
                                                                                      assert(m_states[m_aut.get_transitions()[*it].from()].block->
                                                                                                                                   start_bottom_states==pivot);
                                                                                    }
                                                                                    assert(it<=end_smaller_than_pivot);
                                                                                    for (; it<end_smaller_than_pivot; ++it) {
                                                                                      assert(max_below_pivot<pivot);
                                                                                      sb=m_states[m_aut.get_transitions()[*it].from()].block->
                                                                                                                                           start_bottom_states;
                                                                                      assert(sb>=min_below_pivot);  assert(sb<=max_below_pivot);
                                                                                    }
                                                                                    assert(it==begin_larger_than_pivot);  assert(it<=end_same_BLC);
                                                                                    for (; it<end_same_BLC; ++it) {
                                                                                      assert(pivot<min_above_pivot);
                                                                                      sb=m_states[m_aut.get_transitions()[*it].from()].block->
                                                                                                                                           start_bottom_states;
                                                                                      assert(sb>=min_above_pivot);  assert(sb<=max_above_pivot);
                                                                                    }
                                                                                  }
                                                                                #endif
      if (start_same_BLC<end_equal_to_pivot)
      {
        order_BLC_transitions_single_BLC_set(pivot,
                                           start_same_BLC, end_equal_to_pivot);
      }
      // Now try to use only tail recursion:
      if (min_above_pivot>=max_above_pivot)
      {
        if (begin_larger_than_pivot<end_same_BLC)
        {                                                                       assert(min_above_pivot==max_above_pivot);
          order_BLC_transitions_single_BLC_set(min_above_pivot,
                                        begin_larger_than_pivot, end_same_BLC);
        }
        if (end_equal_to_pivot<begin_larger_than_pivot)
        {
          order_BLC_transitions(end_equal_to_pivot, begin_larger_than_pivot,
                                             min_below_pivot, max_below_pivot);
        }
        return;
      }
      if (min_below_pivot>=max_below_pivot)
      {
        if (end_equal_to_pivot<begin_larger_than_pivot)
        {                                                                       assert(min_below_pivot==max_below_pivot);
          order_BLC_transitions_single_BLC_set(min_below_pivot,
                                  end_equal_to_pivot, begin_larger_than_pivot);
        }
        if (begin_larger_than_pivot<end_same_BLC)
        {
          order_BLC_transitions(begin_larger_than_pivot, end_same_BLC,
                                             min_above_pivot, max_above_pivot);
        }
        return;
      }                                                                         assert(end_equal_to_pivot<begin_larger_than_pivot);
                                                                                assert(min_below_pivot<max_below_pivot);
                                                                                assert(begin_larger_than_pivot<end_same_BLC);
      / * Here we cannot do tail recursion                                  * / assert(min_above_pivot<max_above_pivot);
      order_BLC_transitions(end_equal_to_pivot, begin_larger_than_pivot,
                                             min_below_pivot, max_below_pivot);
      // Hopefully the compiler turns this tail recursion into iteration
      order_BLC_transitions(begin_larger_than_pivot, end_same_BLC,
                                             min_above_pivot, max_above_pivot);
      #undef max_below_pivot
      #undef min_above_pivot
    }
*/

    // Algorithm 5. Stabilize the current partition with respect to the current constellation
    // given that the blocks in m_blocks_with_new_bottom_states do contain new bottom states.
    // Stabilisation is always called after initialisation, i.e., m_aut.get_transitions()[ti].transition refers
    // to a position in m_BLC_transitions, where the transition index of this transition can be found.

    void stabilizeB()
    {
//std::cerr << "Called stabilizeB(). There are " << m_blocks_with_new_bottom_states.size() << " blocks with new bottom states to start with.\n";
      if (m_blocks_with_new_bottom_states.empty())
      {
        return;
      }
      // Qhat contains the slices of BLC transitions that still need stabilization
      // Algorithm 5, Line 5.2
      std::vector<std::pair<BLC_list_iterator, BLC_list_iterator> > Qhat;
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  std::vector<std::pair<BLC_list_const_iterator, BLC_list_const_iterator> >
                                                                                                                          initialize_qhat_work_to_assign_later;
                                                                                  std::vector<std::pair<BLC_list_const_iterator, BLC_list_const_iterator> >
                                                                                                                          stabilize_work_to_assign_later;
                                                                                #endif
      // Algorithm 5, Line 5.3
      for (;;)
      {
        /* Algorithm 5, Line 5.4                                             */ assert(!m_blocks_with_new_bottom_states.empty());
        for(block_type_lb* const bi: m_blocks_with_new_bottom_states)
        {                                                                       assert(bi->contains_new_bottom_states);
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  // The work in this loop is assigned to the (new) bottom states in bi
                                                                                  // It cannot be assigned to the block bi because there may be more new bottom
                                                                                  // states later.
                                                                                  const state_in_block_pointer_lb* new_bott_it=bi->start_bottom_states;
                                                                                  assert(new_bott_it < bi->sta.rt_non_bottom_states);
                                                                                  do
                                                                                  {
                                                                                    mCRL2complexity(new_bott_it->ref_state,
                                                                                              add_work(check_complexity::stabilizeB__prepare_block, 1), *this);
                                                                                  }
          /* Algorithm 5, Line 5.5                                           */   while (++new_bott_it<bi->sta.rt_non_bottom_states);
                                                                                #endif
                                                                                assert(!bi->block_BLC_source->block_to_constellation.empty());
//std::cerr << bi->debug_id(*this) << " has new bottom states";
          if (1>=number_of_states_in_block(*bi))
          {
            // blocks with only 1 state do not need to be stabilized further
//std::cerr << " but it has only 1 state.  Nothing needs to be done for it.\n";
            continue;
          }
          if (!bi->is_small_subblock)
          {
//std::cerr << "; it is a large subblock.\n";
            // Either the subblock is large, or its smallness has been used
            // during an earlier run of this main loop in stabilizeB().  So we
            // now have to treat is as a large subblock, at least for the
            // splitters that we are going to add now.
            // >>> It can happen that the subblock is treated as small for some
            // splitters and as large for others. So we need to store this
            // information together with Qhat.
            // >>> It can also happen that the (original) BLC source of bi
            // contains some blocks that have new bottom states and some that
            // don't. Therefore, make_BLC_simple is not allowed to mark just
            // all transitions that it touches.
            // Algorithm 5, Line 5.6
            make_BLC_simple(*bi);
//if (bi->is_small_subblock) { std::cerr << "Now " << bi->debug_id(*this) << " is a small subblock.\n"; }
            // Algorithm 5, Line 5.7–5.8 are already done in make_BLC_simple()
          }
//else { std::cerr << '\n'; }
        }
        // Algorithm 5, Line 5.9
        for(block_type_lb* const bi: m_blocks_with_new_bottom_states)
        {                                                                       assert(bi->contains_new_bottom_states);
                                                                                // mCRL2complexity(new bottom states in bi, ...)
                                                                                    // This loop runs over exactly the same blocks as the one above, so we do not need a separate counter.
                                                                                assert(!bi->block_BLC_source->block_to_constellation.empty());
          // Algorithm 5, Line 5.10
          bi->contains_new_bottom_states=false;
          if (1>=number_of_states_in_block(*bi))
          {
            continue;
          }
//std::cerr << "Now marking transitions out of " << bi->debug_id(*this) << ".\n";
          simple_list<BLC_indicators_lb>& btc =
                                  bi->block_BLC_source->block_to_constellation;
          // Algorithm 5, Line 5.11
          if (!bi->is_small_subblock)
          {                                                                     assert(bi->block_BLC_source->start_BLC_source==bi->start_bottom_states);
            /* Algorithm 5, Line 5.19                                        */ assert(bi->block_BLC_source->end_BLC_source==bi->end_states);
            typename simple_list<BLC_indicators_lb>::iterator ind=btc.begin();  assert(btc.end() != ind);
            do
            {                                                                   assert(ind->start_same_BLC<ind->end_same_BLC);
              if (!ind->is_stable())
              {
//std::cerr << ind->debug_id(*this) << " is already unstable, no need to handle this or further BLC sets.\n";
                                                                                #ifndef NDEBUG
                                                                                  do {
                                                                                    const transition& tr = m_aut.get_transitions()[*ind->start_same_BLC];
                                                                                    assert(m_states[tr.from()].block == bi);  assert(!ind->is_stable());
                                                                                    assert(m_states[tr.to()].block->constellation!=bi->constellation ||
                                                                                           !is_inert_during_init_if_branching(tr));
                                                                                    // assert(ind is already in Qhat);
                                                                                  } while (++ind != btc.end());
                                                                                #endif
                break;
              }
              const transition& tr = m_aut.get_transitions()
                                                        [*ind->start_same_BLC]; assert(m_states[tr.from()].block == bi);
              const simple_list<BLC_indicators_lb>::iterator
                                                      next_ind = btc.next(ind);
              if(m_states[tr.to()].block->constellation == bi->constellation &&
                 is_inert_during_init_if_branching(tr))
              {
                // this BLC set contains the C-inert transitions. No need to
                // stabilize under it.
//std::cerr << ind->debug_id(*this) << " contains the C-inert transitions, no need to stabilize under it.\n";
                if (btc.begin() != ind)
                {
                  // The stable BLC set(s) need to be at the beginning
                  btc.splice(btc.begin(), btc, ind);
                  // < This will change btc.next(ind) == std::next(ind),
                  // therefore we had to calculate and store next_ind above.
                }
              }
              else
              {
                ind->starts_in_small_subblock = false;
                ind->make_unstable();
                // Algorithm 5, 5.20
                Qhat.emplace_back(ind->start_same_BLC, ind->end_same_BLC);
//std::cerr << "Adding " << ind->debug_id(*this) << " to the list of unstable BLC sets.\n";
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  // The work is assigned to the transitions out of new bottom states in ind.
                                                                                  // Try to find a new bottom state to which to assign it.
                                                                                  bool work_assigned = false;
                                                                                  // assign the work to the transitions out of bottom states in this BLC-set
                                                                                  for (BLC_list_const_iterator work_it = ind->start_same_BLC;
                                                                                                                          work_it<ind->end_same_BLC; ++work_it)
                                                                                  {
                                                                                    // assign the work to this transition
                                                                                    if (0==m_states[m_aut.get_transitions()
                                                                                                     [*work_it].from()].no_of_outgoing_block_inert_transitions)
                                                                                    {
                                                                                      #ifndef NDEBUG
                                                                                        if (work_assigned) {
                                                                                          mCRL2complexity(&m_transitions[*work_it], add_work_notemporary(
                                                                                                     check_complexity::stabilizeB__initialize_Qhat, 1), *this);
                                                                                          continue;
                                                                                        }
                                                                                      #endif
                                                                                      mCRL2complexity(&m_transitions[*work_it], add_work(
                                                                                                     check_complexity::stabilizeB__initialize_Qhat, 1), *this);
                                                                                      work_assigned = true;
                                                                                      #ifdef NDEBUG
                                                                                        break;
                                                                                      #endif
                                                                                    }
                                                                                  }
                                                                                  if (!work_assigned) {
                                                                                    // We register that we still have to find a transition from a new bottom
                                                                                    // state in this slice.
                                                                                    initialize_qhat_work_to_assign_later.emplace_back(ind->start_same_BLC,
                                                                                                                                      ind->end_same_BLC);
                                                                                  }
                                                                                #endif
              }
              ind = next_ind;
            }
            while (btc.end() != ind);

            // Algorithm 5, Line 5.17
            state_in_block_pointer_lb* si=bi->start_bottom_states;              assert(si<bi->sta.rt_non_bottom_states);
            do
            {                                                                   mCRL2complexity(si->ref_state, add_work(
              /* Algorithm 5, Line 5.18                                      */          check_complexity::stabilizeB__distribute_states_over_Phat, 1), *this);
//std::cerr << "Marking outgoing transitions of new bottom " << si->ref_state->debug_id(*this) << ".\n";
              outgoing_transitions_it_lb end_it=
                  std::next(si->ref_state)>=m_states.end()
                        ? m_outgoing_transitions.end()
                        : std::next(si->ref_state)->start_outgoing_transitions; assert(si->ref_state->block==bi);
              for (outgoing_transitions_it_lb ti=
                    si->ref_state->start_outgoing_transitions; ti<end_it; ++ti)
              {                                                                 // mCRL2complexity(&m_transitions[*ti->ref_BLC_transitions],
                                                                                //                 add_work(..., 1), *this);
                const transition& t=                                                // subsumed under the above counter
                             m_aut.get_transitions()[*ti->ref_BLC_transitions]; assert(m_states.begin()+t.from()==si->ref_state);
                if (bi->constellation!=m_states[t.to()].block->constellation ||
                    !is_inert_during_init_if_branching(t))
                {
                  // the transition is not constellation-inert, so mark it
                  mark_BLC_transition(ti);
                }                                                               else {  assert(m_transitions[*ti->ref_BLC_transitions].
                                                                                                       transitions_per_block_to_constellation->is_stable());  }
                /* Actually it's enough to mark one transition per saC slice:*/ assert(ti <= ti->start_same_saC);
                ti = ti->start_same_saC;
              }
              ++si;
            }
            while (si<bi->sta.rt_non_bottom_states);
          }
          else
          {
            bi->is_small_subblock = false;                                      // mCRL2complexity(bi, add_work(check_complexity::use_smallness_of_block,
                                                                                //            check_complexity::log_n-
                                                                                //            check_complexity::ilog2(number_of_states_in_block(*bi))), *this);
            if (bi->start_bottom_states ==
                                      bi->block_BLC_source->start_BLC_source &&
                   bi->end_states == bi->block_BLC_source->end_BLC_source)
            {
//std::cerr << bi->debug_id(*this) << " is small and has a simple BLC source set. We can summarily mark all its outgoing transitions.\n";
              // Because the block has a simple BLC source, marking the
              // transitions can be simplified: all transitions in a BLC set
              // are to be marked, unless it is the BLC set of the C-inert
              // transitions.

              // Algorithm 5, Line 5.14
              typename simple_list<BLC_indicators_lb>::iterator
                                                             ind = btc.begin(); assert(btc.end() != ind);
              do
              {                                                                 // mCRL2complexity(ind, add_work(..., max_B), *this)
                if (!ind->is_stable())                                              // subsumed under the above block counter
                {
//std::cerr << ind->debug_id(*this) << " is already unstable, no need to handle this or further BLC sets.\n";
                                                                                #ifndef NDEBUG
                                                                                  do {
                                                                                    const transition& tr = m_aut.get_transitions()[*ind->start_same_BLC];
                                                                                    assert(m_states[tr.from()].block == bi);  assert(!ind->is_stable());
                                                                                    assert(m_states[tr.to()].block->constellation!=bi->constellation ||
                                                                                           !is_inert_during_init_if_branching(tr));
                                                                                    // assert(ind is already in Qhat);
                                                                                    assert(ind->starts_in_small_subblock); // < I hope this holds, so it is not necessary to do more work.
                                                                                  } while(++ind != btc.end());
                                                                                #endif
                  break;
                }
                const transition& tr = m_aut.get_transitions()
                                                        [*ind->start_same_BLC]; assert(m_states[tr.from()].block == bi);
                const simple_list<BLC_indicators_lb>::iterator
                                                      next_ind = btc.next(ind);
                if(m_states[tr.to()].block->constellation==bi->constellation &&
                   is_inert_during_init_if_branching(tr))
                {                                                               assert(ind->is_stable());
//std::cerr << ind->debug_id(*this) << " contains the C-inert transitions, no need to stabilize under it.\n";
                  // this BLC set contains the C-inert transitions. No need to
                  // stabilize under it.
                  if (btc.begin() != ind)
                  {
                    // The stable BLC set(s) need to be at the beginning
                    btc.splice(btc.begin(), btc, ind);
                    // < This changes std::next(ind) == btc.next(ind),
                    // therefore we had to calculate and store next_ind above.
                  }
                }
                else
                {
//std::cerr << "Summarily mark all transitions in " << ind->debug_id(*this) << ".\n";
                  // Algorithm 5, Lines 5.12–5.13
                  // mark all transitions in the BLC set at once:
                  ind->starts_in_small_subblock = true;
                  ind->start_marked_BLC = ind->start_same_BLC;
                  // Algorithm 5, Line 5.15
                  Qhat.emplace_back(ind->start_same_BLC, ind->end_same_BLC);
                }
                ind = next_ind;
              }
              while (btc.end() != ind);
            }
            else
            {
              // bi is a small subblock, but its super-BLC source contains
              // other block that are likely without new bottom states (because
              // between two new bottom blocks there is always some other
              // block containing old bottom states).  So we  have to go
              // through all transitions of bi and mark them.
              // Algorithm 5, Line 5.12
              state_in_block_pointer_lb* it = bi->start_bottom_states;
              state_in_block_pointer_lb* const end_it = bi->end_states;         assert(it < end_it);
              do
              {                                                                 // mCRL2complexity(it->ref_state, add_work(..., max_B), *this);
                // Algorithm 5, line 5.13                                           // subsumed under the above block counter
                outgoing_transitions_it_lb out_it =
                                     it->ref_state->start_outgoing_transitions;
//std::cerr << "Marking outgoing transitions of "
//<< (it < bi->sta.rt_non_bottom_states ? "new bottom " : "non-bottom ")
//<< it->ref_state->debug_id(*this) << ".\n";
                outgoing_transitions_const_it_lb const
                    out_it_end=std::next(it->ref_state)==m_states.end()
                        ? m_outgoing_transitions.end()
                        : std::next(it->ref_state)->start_outgoing_transitions; assert(out_it < out_it_end);
                do
                {
                  BLC_list_iterator const old_pos=out_it->ref_BLC_transitions;  // mCRL2complexity(&m_transitions[*old_pos], add_work(..., max_B), *this);
                  const transition& tr = m_aut.get_transitions()[*old_pos];         // subsumed under the above block counter
                  if (m_states[tr.to()].block->constellation !=
                                                           bi->constellation ||
                      !is_inert_during_init_if_branching(tr))
                  {
                    // the transition is not C-inert, so mark it

                    simple_list<BLC_indicators_lb>::iterator ind=m_transitions
                             [*old_pos].transitions_per_block_to_constellation;
                    if (ind->is_stable())
                    {
                      ind->make_unstable();
                      ind->starts_in_small_subblock = true;
                      // move ind to the end of the list of BLC sets:
                      btc.splice(btc.end(), btc, ind);
                      // Algorithm 5, Line 5.14–5.15
                      Qhat.emplace_back(ind->start_same_BLC,ind->end_same_BLC);
                    }                                                           assert(ind->starts_in_small_subblock);
                    mark_BLC_transition(out_it);
                  }
                  /* Actually it's enough to mark one transition per saC     */ assert(out_it <= out_it->start_same_saC);
                  // slice:
                  out_it = std::next(out_it->start_same_saC);
                }
                while (out_it < out_it_end);
                ++it;
              }
              while (it < end_it);
            }
          }
          // Algorithm 5, Line 5.16
        }
        // Algorithm 5, Line 5.10
        clear(m_blocks_with_new_bottom_states);

        // Algorithm 5, line 5.21
        // inner loop to be executed until further new bottom states are found:
        do
        {                                                                       assert(m_blocks_with_new_bottom_states.empty());
          if (Qhat.empty())
          {                                                                     assert(check_data_structures("End of stabilizeB()"));
            /* nothing needs to be stabilized any more.                      */ assert(check_stability("End of stabilizeB()"));
                                                                                // Therefore, it is impossible that further new bottom states are
                                                                                // found in these rounds.  So all work must have been accounted for:
                                                                                assert(initialize_qhat_work_to_assign_later.empty());
                                                                                assert(stabilize_work_to_assign_later.empty());
            return;
          }
                                                                                #ifndef NDEBUG
          /* Algorithm 5, line 5.22                                          */   print_data_structures("New bottom state loop");
                                                                                #endif
                                                                                assert(check_data_structures("New bottom state loop", false));
          std::pair<BLC_list_iterator,BLC_list_iterator>& Qhat_elt=Qhat.back(); assert(check_stability("New bottom state loop", &Qhat));
                                                                                assert(Qhat_elt.first<Qhat_elt.second);
          const simple_list<BLC_indicators_lb>::iterator
                    splitter = m_transitions[*std::prev(Qhat_elt.second)].
                                        transitions_per_block_to_constellation; assert(splitter->end_same_BLC==Qhat_elt.second);
          // Algorithm 5, Line 5.23
          Qhat_elt.second=splitter->start_same_BLC;                             assert(splitter->start_same_BLC<splitter->end_same_BLC);
          const transition& first_t=
                            m_aut.get_transitions()[*splitter->start_same_BLC]; assert(!splitter->is_stable());
          block_type_lb& from_block_index=*m_states[first_t.from()].block;      assert(!from_block_index.contains_new_bottom_states);
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  // The work is assigned to the transitions out of new bottom states in splitter.
                                                                                  // (That are marked transitions out of bottom states.)
                                                                                  BLC_list_const_iterator work_it=splitter->start_marked_BLC;
                                                                                  if (work_it==splitter->end_same_BLC && splitter->starts_in_small_subblock) {
                                                                                    // I think that this situation has arisen because of a call to make_BLC_simple()
                                                                                    // that has removed all new bottom state blocks from the BLC source of splitter.
                                                                                    // Then the work should be assigned to some unit that allowed to call make_BLC_simple().
                                                                                    mCRL2log(log::warning) << "Cannot find a way to assign work on " << splitter->debug_id(*this) << '\n';
                                                                                  } else if (work_it!=splitter->end_same_BLC) {
                                                                                    bool work_assigned=false;
                                                                                    do {
                                                                                      // assign the work to this transition
                                                                                      if (0==m_states[m_aut.get_transitions()[*work_it].from()].
                                                                                                                          no_of_outgoing_block_inert_transitions)
                                                                                      {
                                                                                        #ifndef NDEBUG
                                                                                          if (work_assigned) {
                                                                                            mCRL2complexity(&m_transitions[*work_it], add_work_notemporary(
                                                                                                           check_complexity::stabilizeB__main_loop, 1), *this);
                                                                                            continue;
                                                                                          }
                                                                                        #endif
                                                                                        mCRL2complexity(&m_transitions[*work_it],
                                                                                                  add_work(check_complexity::stabilizeB__main_loop, 1), *this);
                                                                                        work_assigned=true;
                                                                                        #ifdef NDEBUG
                                                                                          break;
                                                                                        #endif
                                                                                      }
                                                                                    } while (++work_it!=splitter->end_same_BLC);
                                                                                    if (!work_assigned) {
                                                                                      // We register that we still have to find a transition from a new bottom
                                                                                      // state in this slice.
                                                                                      stabilize_work_to_assign_later.emplace_back(splitter->start_same_BLC,
                                                                                                                                splitter->end_same_BLC);
                                                                                    }
                                                                                  }
                                                                                #endif
          BLC_source_type& BLC_source=*from_block_index.block_BLC_source;
          if (std::distance(BLC_source.start_BLC_source,
                                               BLC_source.end_BLC_source) <= 1)
          {
            // a block with 1 state does not need to be split
            // We still need to make the splitter stable because it is
            // expected that all BLC sets are stable by the main/co-split
            // phase.
            // The BLC set will not be inserted into Qhat another time, so
            // it is not necessary to maintain the order of BLC sets (first
            // stable ones, then unstable ones).
            splitter->make_stable();
          }
          else
          {
                                                                                #ifndef NDEBUG
            /* Algorithm 5, Line 5.24                                        */   bool is_inert = is_inert_during_init_if_branching(first_t);
                                                                                  constellation_type_lb& to_constellation =
            /* we need to set up a list of blocks that need to be stabilized */                                   *m_states[first_t.to()].block->constellation;
                                                                                #endif
            std::forward_list<block_that_needs_refinement_type>
                                                   blocks_that_need_refinement;

            // Algorithm 5, Line 5.26
            // go through the *marked* transitions in the BLC set and set the
            // block's ReachAlw etc., similar to refine_super_BLC

            for(BLC_list_iterator splitter_it = splitter->start_marked_BLC;
                          splitter_it != splitter->end_same_BLC; ++splitter_it)
            {                                                                   // mCRL2complexity(&m_transitions[*splitter_it], add_work(...), *this);
              const transition& t=m_aut.get_transitions()[*splitter_it];        assert(is_inert == is_inert_during_init(t));
              state_in_block_pointer_lb const src = m_states.begin()+t.from();  assert(m_states[t.to()].block->constellation == &to_constellation);
              // block bi satisfies basic preconditions of refinability
              block_type_lb& bi = *src.ref_state->block;                        assert(!is_inert || bi.constellation != &to_constellation);
                                                                                assert(!bi.contains_new_bottom_states);
              if (1 < number_of_states_in_block(bi))
              {
                if (nullptr == bi.refinement_info)
                {
                  // block has not yet been hit by the procedure
                  blocks_that_need_refinement.emplace_front(bi,
                    splitter->starts_in_small_subblock ? nullptr : &*splitter); assert(nullptr!=bi.refinement_info);
                  // Algorithm 5, Line 5.28
                  blocks_that_need_refinement.front().start_bottom_states
                                         [AvoidSml+1] = bi.start_bottom_states;
                }
                block_that_needs_refinement_type& bri = *bi.refinement_info;    assert(bri.start_bottom_states[AvoidSml] == bri.start_bottom_states[AvoidLrg]);
                if (0 == src.ref_state->no_of_outgoing_block_inert_transitions)
                {                                                               assert(bi.start_bottom_states<=src.ref_state->ref_states_in_blocks);
                  /* src is a ReachAlw-bottom state                          */ assert(src.ref_state->ref_states_in_blocks<bi.sta.rt_non_bottom_states);
                  if (src.ref_state->ref_states_in_blocks <
                                             bri.start_bottom_states[AvoidSml])
                  {
                    // source state is already in ReachAlw-bottom
                  }
                  else
                  {
                    /* Algorithm 5, Line 5.27: state belongs to ReachAlw     */ static_assert(ReachAlw + 1 == AvoidSml);
                    swap_states_in_states_in_block
                                         (bri.start_bottom_states[AvoidSml],
                                          src.ref_state->ref_states_in_blocks);
                    bri.start_bottom_states[AvoidLrg] =
                                           ++bri.start_bottom_states[AvoidSml];
                  }
                }
                else
                {                                                               assert(splitter->starts_in_small_subblock);
                                                                                assert(nullptr == bri.large_splitter);
                  /* src has outgoing tau transitions; it might end in the   */ assert(bi.sta.rt_non_bottom_states<=src.ref_state->ref_states_in_blocks);
                  /* NewBotSt-subblock.                                      */ assert(src.ref_state->ref_states_in_blocks<bi.end_states);
                  if (undefined == src.ref_state->counter)
                  {
                    // Algorithm 5, Line 5.32: initialize counter
                    src.ref_state->counter = marked(ReachAlw) +
                         src.ref_state->no_of_outgoing_block_inert_transitions; assert(is_in_marked_range_of(src.ref_state->counter, ReachAlw));
                    // Algorithm 5, Line 5.30: state belongs to pot-ReachAlw
                    bri.potential_non_bottom_states[ReachAlw].push_back(src);
                  }                                                             else {
                                                                                  assert(is_in_marked_range_of(src.ref_state->counter, ReachAlw));
                                                                                  assert(std::find(bri.potential_non_bottom_states[ReachAlw].begin(),
                                                                                                   bri.potential_non_bottom_states[ReachAlw].end(), src)!=
                                                                                                              bri.potential_non_bottom_states[ReachAlw].end());
                                                                                }
                }
              }                                                                 else  {  assert(nullptr==bi.refinement_info);  }
            }
            if (splitter->starts_in_small_subblock)
            {
              make_stable_and_move_to_start_of_BLC(BLC_source, splitter);
            }
            else
            {                                                                   assert(BLC_source.start_BLC_source == from_block_index.start_bottom_states);
              /* splitter->make_stable(); should be called only later, as    */ assert(BLC_source.end_BLC_source == from_block_index.end_states);
              // four_way_splitB may still have to go through the *unmarked*
              // transitions of the splitter.
              // Algorithm 5, Line 5.24
              if (blocks_that_need_refinement.empty())
              {                                                                 assert(!from_block_index.contains_new_bottom_states);
                /* The block needs to be refined but has no marked           */ assert(number_of_states_in_block(from_block_index) > 1);
                // transitions.
                /* Algorithm 5, Line 5.25                                    */ assert(!is_inert || from_block_index.constellation != &to_constellation);
                blocks_that_need_refinement.emplace_front(from_block_index,
                                                                   &*splitter);
                blocks_that_need_refinement.front().start_bottom_states
                           [AvoidSml+1] = from_block_index.start_bottom_states;
              }                                                                 assert(std::next(blocks_that_need_refinement.begin()) ==
                                                                                                                            blocks_that_need_refinement.end());
            }

            // Algorithm 5, Line 5.34: for all blocks bi hit by the above loop
            while(!blocks_that_need_refinement.empty())
            {
              block_that_needs_refinement_type& bri=
                                           blocks_that_need_refinement.front();
              block_type_lb& bi=*bri.start_bottom_states[0]->ref_state->block;  assert(!bi.contains_new_bottom_states);
              bool bi_was_small_subblock = nullptr==bri.large_splitter;         assert(1<number_of_states_in_block(bi));
              // Algorithm 5, Line 5.35: Call four_way_splitB(bi)
              four_way_splitB(bri);                                             assert(bi_was_small_subblock == (nullptr==bri.large_splitter));
              blocks_that_need_refinement.pop_front();
              bi.refinement_info = nullptr;
              /* Algorithm 5, Line 5.36                                      */ assert(&BLC_source==bi.block_BLC_source);
              if (!bi_was_small_subblock)
              {
                make_stable_and_move_to_start_of_BLC(BLC_source, splitter);     assert(&bi==&from_block_index);
                /* Algorithm 5, Line 5.37–5.38: B_lrg is now bi              */ assert(blocks_that_need_refinement.empty());
                if (!bi.is_small_subblock)
                {
                  // bi was not a small subblock before. That means that its
                  // BLC source only contained bi, i.e. it only contained
                  // block(s) with new bottom states.  Then make_BLC_simple()
                  // should mark all transitions in unstable small BLC sets
                  // that it creates.
                  make_BLC_simple(bi, true);                                    assert(&BLC_source == bi.block_BLC_source);
                  // As bi was not a small subblock, also `make_blc_simple()`
                  // will not turn it into a small subblock.
                }
                else
                {                                                               assert(!BLC_source.block_to_constellation.empty());
                  // All subblocks that have been generated are small.  We now
                  // can mark all transitions in their BLC sets, as far as they
                  // are still unstable.
                  simple_list<BLC_indicators_lb>::iterator btc_it =
                                BLC_source.block_to_constellation.before_end();
                  if (btc_it->is_stable())
                  {
                    // this was the last splitter for this blc_src; we do not
                    // need to split more.  Therefore it is also not
                    // necessary to declare blocks large.
                  }
                  else
                  {
                    // Algorithm 5, Line 5.40 / 5.42
                    // mark all outgoing transitions of blc_src in Qhat
                    do
                    {
                      btc_it->start_marked_BLC = btc_it->start_same_BLC;
                      btc_it->starts_in_small_subblock = true;                  assert(btc_it != BLC_source.block_to_constellation.begin());
                      --btc_it;
                    }
                    while (!btc_it->is_stable());
                    // make the blocks in BLC_source large, as we have used
                    // their smallness to mark the transitions.
                    // However, we will not make blocks with new bottom states
                    // large, as they will soon be handled in the next
                    // iteration of the outer loop.
                    state_in_block_pointer_lb* blc_src_it =
                                                   BLC_source.start_BLC_source; assert(blc_src_it < BLC_source.end_BLC_source);
                    do
                    {
                      block_type_lb& current_blk=*blc_src_it->ref_state->block;
                      current_blk.is_small_subblock =
                                        current_blk.contains_new_bottom_states; assert(!current_blk.contains_new_bottom_states ||
                                                                                       std::find(m_blocks_with_new_bottom_states.begin(),
                                                                                                 m_blocks_with_new_bottom_states.end(), &current_blk)!=
                                                                                                                        m_blocks_with_new_bottom_states.end());
                      blc_src_it = current_blk.end_states;                      assert(blc_src_it <= BLC_source.end_BLC_source);
                    }
                    while (blc_src_it < BLC_source.end_BLC_source);
                  }
                }
                // If the subblock is large, there can be only one block in
                // `blocks_that_need_refinement`, so we can leave immediately
                // after handling it:
                break;
              }                                                                 else  {  assert(splitter->is_stable());  }
            }
          }                                                                     assert(Qhat_elt.first<=Qhat_elt.second);
          if (Qhat_elt.first==Qhat_elt.second)
          {
            Qhat.pop_back(); // invalidates Qhat_elt
          }
          // Algorithm 3, Line 3.13
        }
        while (m_blocks_with_new_bottom_states.empty());
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  // Further new bottom states have been found, so we now have a chance at
                                                                                  // assigning the initialization of Qhat that had not yet been assigned
                                                                                  // earlier.
                                                                                  for (std::vector<std::pair<BLC_list_const_iterator,BLC_list_const_iterator> >
                                                                                            ::iterator qhat_it=initialize_qhat_work_to_assign_later.begin();
                                                                                                         qhat_it!=initialize_qhat_work_to_assign_later.end(); )
                                                                                  {
                                                                                    bool new_bottom_state_with_transition_found=false;
                                                                                    for (BLC_list_const_iterator work_it=qhat_it->first;
                                                                                                                            work_it<qhat_it->second; ++work_it)
                                                                                    {
                                                                                      const state_index t_from=m_aut.get_transitions()[*work_it].from();
                                                                                      if (0==m_states[t_from].no_of_outgoing_block_inert_transitions &&
                                                                                          m_states[t_from].block->contains_new_bottom_states)
                                                                                      {
                                                                                        // t_from is a new bottom state, so we can assign the work to this
                                                                                        // transition
                                                                                        #ifndef NDEBUG
                                                                                          if (new_bottom_state_with_transition_found)
                                                                                          {
                                                                                            mCRL2complexity(&m_transitions[*work_it], add_work_notemporary
                                                                                                           (check_complexity::
                                                                                                            stabilizeB__initialize_Qhat_afterwards, 1), *this);
                                                                                            continue;
                                                                                          }
                                                                                        #endif
                                                                                        mCRL2complexity(&m_transitions[*work_it], add_work(check_complexity::
                                                                                                            stabilizeB__initialize_Qhat_afterwards, 1), *this);
                                                                                        new_bottom_state_with_transition_found=true;
                                                                                        #ifdef NDEBUG
                                                                                          break;
                                                                                        #endif
                                                                                      }
                                                                                    }
                                                                                    if (new_bottom_state_with_transition_found)
                                                                                    {
                                                                                      // The work has been assigned successfully, so we can replace this
                                                                                      // entry of initialize_qhat_work_to_assign_later with the last one.
                                                                                      if (std::next(qhat_it)==initialize_qhat_work_to_assign_later.end())
                                                                                      {
                                                                                        initialize_qhat_work_to_assign_later.pop_back();
                                                                                        break;
                                                                                      }
                                                                                      else
                                                                                      {
                                                                                        *qhat_it=initialize_qhat_work_to_assign_later.back();
                                                                                        initialize_qhat_work_to_assign_later.pop_back();
                                                                                      }
                                                                                    }
                                                                                    else
                                                                                    {
                                                                                      ++qhat_it;
                                                                                    }
                                                                                  }

                                                                                  // We shall also try and find further new bottom states to which to assign
                                                                                  // the main loop iterations that had not yet been assigned earlier.
                                                                                  for (std::vector<std::pair<BLC_list_const_iterator,BLC_list_const_iterator> >
                                                                                            ::iterator stabilize_it=stabilize_work_to_assign_later.begin();
                                                                                                          stabilize_it!=stabilize_work_to_assign_later.end(); )
                                                                                  {
                                                                                    bool new_bottom_state_with_transition_found=false;
                                                                                    for (BLC_list_const_iterator work_it=stabilize_it->first;
                                                                                                                       work_it<stabilize_it->second; ++work_it)
                                                                                    {
                                                                                      const state_index t_from=m_aut.get_transitions()[*work_it].from();
                                                                                      if (0==m_states[t_from].no_of_outgoing_block_inert_transitions &&
                                                                                          m_states[t_from].block->contains_new_bottom_states)
                                                                                      {
                                                                                        // t_from is a new bottom state, so we can assign the work to this
                                                                                        // transition
                                                                                        #ifndef NDEBUG
                                                                                          if (new_bottom_state_with_transition_found)
                                                                                          {
                                                                                            mCRL2complexity(&m_transitions[*work_it], add_work_notemporary(
                                                                                                check_complexity::stabilizeB__main_loop_afterwards, 1), *this);
                                                                                            continue;
                                                                                          }
                                                                                        #endif
                                                                                        mCRL2complexity(&m_transitions[*work_it], add_work(check_complexity::
                                                                                                                  stabilizeB__main_loop_afterwards, 1), *this);
                                                                                        new_bottom_state_with_transition_found=true;
                                                                                        #ifdef NDEBUG
                                                                                          break;
                                                                                        #endif
                                                                                      }
                                                                                    }
                                                                                    if (new_bottom_state_with_transition_found)
                                                                                    {
                                                                                      // The work has been assigned successfully, so we can replace this
                                                                                      // entry of stabilize_work_to_assign_later with the last one.
                                                                                      if (std::next(stabilize_it) == stabilize_work_to_assign_later.end())
                                                                                      {
                                                                                        stabilize_work_to_assign_later.pop_back();
                                                                                        break;
                                                                                      }
                                                                                      else
                                                                                      {
                                                                                        *stabilize_it=stabilize_work_to_assign_later.back();
                                                                                        stabilize_work_to_assign_later.pop_back();
                                                                                      }
                                                                                    }
                                                                                    else
                                                                                    {
                                                                                      ++stabilize_it;
                                                                                    }
                                                                                  }
                                                                                #endif
      }                                                                         assert(0); // unreachable
    }

// =================================================================================================================================
//
//   refine_super_BLC.
//
// =================================================================================================================================

    /// \brief refine all predecessors of a super-BLC set
    void refine_super_BLC(BLC_indicators_lb& small_splitter,
                          BLC_indicators_lb* const large_splitter = nullptr)
    {
//std::cerr << "refine_super_BLC(" << small_splitter.debug_id(*this);
//if (nullptr != large_splitter) { std::cerr << ',' << large_splitter->debug_id(*this); }
//std::cerr << ")\n";
      // The pseudocode for this procedure was not operational but gave a kind
      // of specification of the sets to be constructed.  The comments refer to
      // the most related specification line, but the flow of execution is
      // often different from the pseudocode.
      //
      // In the execution, we go through all transitions in the small splitter
      // and mark the source states accordingly.

      const transition& first_t =
                       m_aut.get_transitions()[*small_splitter.start_same_BLC];
      constellation_type_lb* const new_constellation =
                                   m_states[first_t.to()].block->constellation;
      constellation_type_lb* const old_constellation =
       nullptr==large_splitter ? nullptr :
       m_states[m_aut.get_transitions()[*large_splitter->start_same_BLC].to()].
                                                          block->constellation;
      const bool is_inert = is_inert_during_init(first_t);
        /* 1. All transitions in the main splitter are looked through.       */
        /*    For each state with a transition in the main splitter, it      */
        /*    is possible to check whether it has a transition in the        */
        /*    co-splitter or not, using the `start_same_saC` pointer.        */
        /*    We distribute the states as described above:                   */
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
        /*    - bottom states are moved to ReachAlw, AvoidLrg, or AvoidSml,  */
        /*      depending on the transitions to the main and co-splitter     */
        /*    - non-bottom states are moved to potentially-ReachAlw if they  */
        /*      have a transition in all splitters provided.  (If there is   */
        /*      no co-splitter, that means: if they have a transition in the */
        /*      main splitter.)                                              */
        /*    - non-bottom with a transition in the main splitter but not in */
        /*      the co-splitter, even though the latter is provided, are     */
        /*      moved to HitSmall temporarily.  They will not become part of */   const unsigned char max_C=check_complexity::log_n-check_complexity::
        /*      AvoidSml.                                                    */                   ilog2(number_of_states_in_constellation(*new_constellation));

        /*    The running time for this is assigned to the transitions in    */
        /*    the main splitter, which contains transitions to the new small */   mCRL2complexity(&small_splitter, add_work(check_complexity::
        /*    constellation.                                                 */           four_way_splitB__handle_transitions_in_main_splitter, max_C), *this);
                                                                                #endif
      std::forward_list<block_that_needs_refinement_type>
                                                   blocks_that_need_refinement;
      BLC_list_iterator splitter_it = small_splitter.start_same_BLC;            assert(splitter_it != small_splitter.end_same_BLC);
      do
      {                                                                         // mCRL2complexity(&m_transitions[*splitter_it], add_work(...), *this);
        const transition& t=m_aut.get_transitions()[*splitter_it];                  // subsumed under the above counter
//std::cerr << "Handling " << m_transitions[*splitter_it].debug_id(*this) << '\n';
        state_in_block_pointer_lb const src = m_states.begin() + t.from();      assert(is_inert == is_inert_during_init(t));
        // Algorithm 2, Line 2.2: block bi satisfies basic preconditions of
        // refinability
        block_type_lb& bi = *src.ref_state->block;
        if (!bi.contains_new_bottom_states &&
            1 < number_of_states_in_block(bi) &&
            (!is_inert || bi.constellation != new_constellation))
        {
          if (nullptr == bi.refinement_info)
          {
            // block has not yet been hit by the procedure
//std::cerr << "  Creating new refinement info for " << bi.debug_id(*this) << '\n';
            blocks_that_need_refinement.emplace_front(bi,
                  is_inert && old_constellation == bi.constellation
                                                   ? nullptr : large_splitter); assert(nullptr!=bi.refinement_info);
          }
          block_that_needs_refinement_type& bri = *bi.refinement_info;
          if (0==src.ref_state->no_of_outgoing_block_inert_transitions)
          {                                                                     assert(bi.start_bottom_states<=src.ref_state->ref_states_in_blocks);
            /* src is a ReachAlw-bottom state or an AvoidLrg-bottom state    */ assert(src.ref_state->ref_states_in_blocks<bi.sta.rt_non_bottom_states);
            if (src.ref_state->ref_states_in_blocks <
                                             bri.start_bottom_states[AvoidSml])
            {
//std::cerr << "  " << src.ref_state->debug_id(*this) << " is already in ReachAlw-bottom\n";
                                                                                #ifndef NDEBUG
                                                                                  if (nullptr != large_splitter &&
                                                                                      !(is_inert && old_constellation == bi.constellation)) {
              /* source state is already in ReachAlw-bottom                  */     assert(next_target_constln_in_same_saC(src, splitter_it)==large_splitter);
                                                                                  }
                                                                                #endif
            }
            else if (nullptr == large_splitter /* needed for correctness */ ||
                     (is_inert && old_constellation == bi.constellation))
            {
//std::cerr << "  " << src.ref_state->debug_id(*this) << " is added to ReachAlw-bottom now (without large splitter)\n";
              /* Algorithm 2, Line 2.4: state belongs to ReachAlw            */ static_assert(ReachAlw + 1 == AvoidSml);
              swap_states_in_states_in_block(bri.start_bottom_states[AvoidSml],
                                          src.ref_state->ref_states_in_blocks);
              ++bri.start_bottom_states[AvoidSml];
            }
            else if (bri.start_bottom_states[AvoidSml+1] <=
                                           src.ref_state->ref_states_in_blocks)
            {
//std::cerr << "  " << src.ref_state->debug_id(*this) << " is already in AvoidLrg-bottom\n";
                                                                                #ifndef NDEBUG
                                                                                  assert(nullptr!=large_splitter);
                                                                                  outgoing_transitions_const_it_lb const out_it_end=
              /* source state is already in AvoidLrg-bottom                  */      std::next(src.ref_state)>=m_states.end() ? m_outgoing_transitions.end()
                                                                                                        : std::next(src.ref_state)->start_outgoing_transitions;
                                                                                  for (outgoing_transitions_const_it_lb out_it=
                                                                                       src.ref_state->start_outgoing_transitions; out_it!=out_it_end; ++out_it)
                                                                                  {
                                                                                    assert(m_transitions[*out_it->ref_BLC_transitions].
                                                                                                       transitions_per_block_to_constellation!=large_splitter);
                                                                                  }
                                                                                #endif
            }
            else if (next_target_constln_in_same_saC(src, splitter_it)==
                                                                large_splitter)
            {
//std::cerr << "  " << src.ref_state->debug_id(*this) << " is added to ReachAlw-bottom now (with large splitter)\n";
              /* Algorithm 2, Line 2.8: state belongs to ReachAlw            */ static_assert(ReachAlw + 1 == AvoidSml);
              swap_states_in_states_in_block(bri.start_bottom_states[AvoidSml],
                                          src.ref_state->ref_states_in_blocks);
              ++bri.start_bottom_states[AvoidSml];
            }
            else
            {
//std::cerr << "  " << src.ref_state->debug_id(*this) << " is added to AvoidLrg-bottom now\n";
              /* Algorithm 2, Line 2.9: state belongs to AvoidLrg            */ static_assert(AvoidSml + 1 == AvoidLrg);
              --bri.start_bottom_states[AvoidSml+1];
              swap_states_in_states_in_block
                                         (bri.start_bottom_states[AvoidSml+1],
                                          src.ref_state->ref_states_in_blocks);
            }
          }
          else
          {
            /* src has outgoing tau transitions; it might end in the         */ assert(bi.sta.rt_non_bottom_states<=src.ref_state->ref_states_in_blocks);
            /* NewBotSt-subblock.                                            */ assert(src.ref_state->ref_states_in_blocks<bi.end_states);
            if (undefined == src.ref_state->counter)
            {
              if (nullptr==large_splitter /* needed for correctness */ ||
                  (is_inert && old_constellation == bi.constellation) ||
                  next_target_constln_in_same_saC(src, splitter_it)==
                                                                large_splitter)
              {
//std::cerr << "  " << src.ref_state->debug_id(*this) << " is added to pot-ReachAlw now\n";
                // Algorithm 2, Line 2.15: initialize counter
                src.ref_state->counter = marked(ReachAlw) +
                         src.ref_state->no_of_outgoing_block_inert_transitions; assert(is_in_marked_range_of(src.ref_state->counter, ReachAlw));
                // Algorithm 2, Line 2.5 or 2.10: state belongs to pot-ReachAlw
                bri.potential_non_bottom_states[ReachAlw].push_back(src);
              }
              else
              {
//std::cerr << "  " << src.ref_state->debug_id(*this) << " is added to HitSmall now\n";
                // Algorithm 2, Line 2.11: state belongs to HitSmall
                src.ref_state->counter = marked_HitSmall;
                bri.potential_non_bottom_states_HitSmall.push_back(src);
                                                                                #ifndef NDEBUG
                                                                                  outgoing_transitions_const_it_lb const out_it_end=std::next(src.ref_state)>=
                                                                                         m_states.end() ? m_outgoing_transitions.end()
                                                                                                        : std::next(src.ref_state)->start_outgoing_transitions;
                                                                                  for (outgoing_transitions_const_it_lb out_it=src.ref_state->
                                                                                                      start_outgoing_transitions; out_it!=out_it_end; ++out_it)
                                                                                  {
                                                                                    assert(m_transitions[*out_it->ref_BLC_transitions].
                                                                                                       transitions_per_block_to_constellation!=large_splitter);
                                                                                  }
                                                                                #endif
              }
            }
                                                                                #ifndef NDEBUG
                                                                                  else if (marked_HitSmall==src.ref_state->counter) {
//std::cerr << "  " << src.ref_state->debug_id(*this) << " is already in HitSmall\n";
                                                                                    assert(nullptr != bri.large_splitter);
                                                                                  } else {
//std::cerr << "  " << src.ref_state->debug_id(*this) << " is already in pot-ReachAlw\n";
                                                                                    assert(is_in_marked_range_of(src.ref_state->counter, ReachAlw));
                                                                                    if (nullptr != bri.large_splitter) {
                                                                                      assert(next_target_constln_in_same_saC(src,splitter_it)==bri.large_splitter);
                                                                                    }
                                                                                  }
                                                                                #endif
          }
        }                                                                       else  {  assert(nullptr==bi.refinement_info);  }
        ++splitter_it;
      }
      while (splitter_it!=small_splitter.end_same_BLC);

      /* Algorithm 2, Line 2.16: for all blocks bi hit by the above loop     */ assert(m_BLC_indicators_to_be_deleted.empty());
      while(!blocks_that_need_refinement.empty())
      {
        block_that_needs_refinement_type&
                                       bri=blocks_that_need_refinement.front(); // every entry in blocks_that_need_refinement is generated by some iteration
                                                                                // of the loop above, so there is no need to add a separate work counter.
        block_type_lb& bi = *bri.start_bottom_states[0]->ref_state->block;      assert(!bi.contains_new_bottom_states);
                                                                                assert(1<number_of_states_in_block(bi));
        // Algorithm 2, Line 2.17: Call four_way_splitB(bi)
        four_way_splitB(bri, old_constellation, new_constellation);
        blocks_that_need_refinement.pop_front();
        bi.refinement_info = nullptr;
      }

      for (std::vector<std::pair<BLC_source_type&, simple_list<BLC_indicators_lb>::iterator> >::iterator
                                 it=m_BLC_indicators_to_be_deleted.begin();
                                 it<m_BLC_indicators_to_be_deleted.end(); ++it)
      {                                                                         assert(it->second->start_same_BLC==it->second->end_same_BLC);
                                                                                // the work in this loop can be attributed to the operation that added this BLC
        it->first.block_to_constellation.erase(it->second);                     // set to m_BLC_indicators_to_be_deleted
      }
      clear(m_BLC_indicators_to_be_deleted);
    }


    void create_initial_partition()
    {
      mCRL2log(log::verbose) << "An O(m log n) "
           << (m_branching ? (m_preserve_divergence
                                         ? "divergence-preserving branching "
                                         : "branching ")
                         : "")
           << "bisimulation partitioner created for " << m_aut.num_states()
           << " states and " << m_transitions.size()
           << " transitions (using the experimental algorithm with lazy BLC sets).\n";
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
      /* Algorithm 1, Line 1.2                                               */   check_complexity::init(2 * m_aut.num_states());
                                                                                  // we need ``2*'' because there is one additional call to splitB during initialisation
                                                                                #endif
      group_transitions_on_tgt_label(m_aut);

      /* Count the number of occurring action labels.                        */ assert((unsigned) m_preserve_divergence <= 1);
      mCRL2log(log::verbose) << "Start initialisation of the BLC list in the "
                                            "initialisation, after sorting.\n";
      constellation_type_lb& initial_constellation = *
          #ifdef USE_POOL_ALLOCATOR
             simple_list<BLC_indicators_lb>::get_pool().
             template construct<constellation_type_lb>
          #else
             new constellation_type_lb
          #endif
                    (m_states_in_blocks.data(), m_states_in_blocks.data_end()); assert(1==no_of_constellations);
      BLC_source_type& initial_BLC_source = *
          #ifdef USE_POOL_ALLOCATOR
             simple_list<BLC_indicators_lb>::get_pool().
             template construct<BLC_source_type>
          #else
             new BLC_source_type
          #endif
                    (m_states_in_blocks.data(), m_states_in_blocks.data_end());
      block_type_lb& initial_block = *
          #ifdef USE_POOL_ALLOCATOR
             simple_list<BLC_indicators_lb>::get_pool().
             template construct<block_type_lb>
          #else
             new block_type_lb
          #endif
                     (m_states_in_blocks.data(), m_states_in_blocks.data_end(),
                            m_states_in_blocks.data_end(),
                                    initial_constellation, initial_BLC_source); assert(1==no_of_blocks);
      {
        std::vector<label_index> todo_stack_actions;
        std::vector<transition_index> count_transitions_per_action
             (m_aut.num_action_labels() + (unsigned) m_preserve_divergence, 0);
        for (transition_index ti=0; ti<m_transitions.size(); ++ti)
        {                                                                       // mCRL2complexity(&m_transitions[ti], add_work(..., 1), *this);
          const transition& t=m_aut.get_transitions()[ti];                          // Because every transition is touched exactly once, we do not
//std::cerr << "Counting the label of " << m_transitions[ti].debug_id(*this) << '\n';
          const label_index label=label_or_divergence(t,                            // store a physical counter for this.
                                                    m_aut.num_action_labels()); assert(m_aut.apply_hidden_label_map(t.label())==t.label());
          transition_index& c=count_transitions_per_action[label];
          if (c==0)
          {
//std::cerr << "todo_stack_actions.push_back(" << pp(m_aut.action_label(label)) << ")\n";
            todo_stack_actions.push_back(label);
          }
          c++;
        }
        accumulate_entries(count_transitions_per_action, todo_stack_actions);
        for (transition_index ti=0; ti<m_transitions.size(); ++ti)
        {                                                                       // mCRL2complexity(&m_transitions[ti], add_work(..., 1), *this);
          const transition& t=m_aut.get_transitions()[ti];                          // Because every transition is touched exactly once, we do not store a
          const label_index label = label_or_divergence(t,                          // physical counter for this.
                                                    m_aut.num_action_labels());
          transition_index& c=count_transitions_per_action[label];              assert(c < m_transitions.size());
          m_BLC_transitions[c]=ti;
          c++;
        }

        // create BLC_indicators_lb for every action label:
//std::cerr << "Create BLC_indicators_lb for " << todo_stack_actions.size() << " action labels\n";
        std::vector<label_index>::const_iterator
                                               a_it=todo_stack_actions.begin();
        BLC_list_iterator start_index=m_BLC_transitions.data();
        while (todo_stack_actions.end() != a_it)
        {                                                                       // mCRL2complexity(..., add_work(..., 1), *this);
          const label_index a = *a_it;                                              // not needed because the inner loop is always executed
//std::cerr << "  current label: " << pp(m_aut.action_label(a)) << '\n';
          const BLC_list_iterator end_index =
                      m_BLC_transitions.data()+count_transitions_per_action[a]; assert(end_index<=m_BLC_transitions.data_end());
          // create a BLC_indicator and insert it into the list...
          initial_BLC_source.block_to_constellation.emplace_back
                                                (start_index, end_index, true); assert(start_index<end_index);
          start_index = end_index;
          ++a_it;
        }                                                                     assert(start_index==m_BLC_transitions.data_end());
        // destroy and deallocate `todo_stack_actions` and
        // `count_transitions_per_action` here.
      }

      // Group transitions per outgoing state.
      mCRL2log(log::verbose) << "Start setting outgoing transitions\n";
      {
        fixed_vector<transition_index> count_outgoing_transitions_per_state
                                                       (m_aut.num_states(), 0);
        for(const transition& t: m_aut.get_transitions())
        {                                                                       // mCRL2complexity(&m_transitions[std::distance
                                                                                //             (m_aut.get_transitions().data(), &t)], add_work(..., 1), *this);
          count_outgoing_transitions_per_state[t.from()]++;                         // Because every transition is touched exactly once,
          if (is_inert_during_init(t))                                              // we do not store a physical counter for this.
          {
            m_states[t.from()].no_of_outgoing_block_inert_transitions++;
          }
        }

        // We now set the outgoing transition per state pointer to the first
        // non-inert transition.
        // The counters for outgoing transitions calculated above are reset to
        // 0 and will later contain the number of transitions already stored.
        // Every time an *inert* transition is stored, the outgoing transition
        // per state pointer is reduced by one.
        outgoing_transitions_it_lb
                 current_outgoing_transitions = m_outgoing_transitions.begin();

        // place transitions and set pointers to incoming/outgoing transitions
        for (state_index s=0; s<m_aut.num_states(); ++s)
        {                                                                       // mCRL2complexity(&m_states[s], add_work(..., 1), *this);
          if (marked_range<=m_states[s].no_of_outgoing_block_inert_transitions)     // Because every state is touched exactly once,
          {                                                                         // we do not store a physical counter for this.
            mCRL2log(log::error) << "State " << s << " has "
                    << m_states[s].no_of_outgoing_block_inert_transitions
                    << " outgoing block-inert transitions.  However, the "
                       "four-way-split can handle at most "
                    << (marked_range-1)
                    << " outgoing block-inert transitions per state.  "
                       "Aborting now.\n";
            exit(EXIT_FAILURE);
          }
          m_states[s].start_outgoing_transitions=current_outgoing_transitions+
                            m_states[s].no_of_outgoing_block_inert_transitions;
          current_outgoing_transitions+=
                                       count_outgoing_transitions_per_state[s];
          count_outgoing_transitions_per_state[s]=0;
          // meaning of this counter changes to: number of outgoing transitions
          // already stored
        }                                                                       assert(m_outgoing_transitions.end()==current_outgoing_transitions);

        mCRL2log(log::verbose) << "Moving incoming and outgoing transitions\n";

        for (BLC_list_iterator ti=m_BLC_transitions.data();
                                         ti<m_BLC_transitions.data_end(); ++ti)
        {                                                                       // mCRL2complexity(&m_transitions[*ti], add_work(..., 1), *this);
          const transition& t=m_aut.get_transitions()[*ti];                         // Because every transition is touched exactly once,
          if (is_inert_during_init(t))                                              // we do not store a physical counter for this.
          {
            m_transitions[*ti].ref_outgoing_transitions =
                               --m_states[t.from()].start_outgoing_transitions;
          }
          else
          {
            m_transitions[*ti].ref_outgoing_transitions =
                        m_states[t.from()].start_outgoing_transitions +
                                count_outgoing_transitions_per_state[t.from()];
          }
          m_transitions[*ti].ref_outgoing_transitions->ref_BLC_transitions=ti;
          ++count_outgoing_transitions_per_state[t.from()];
        }
        // destroy and deallocate count_outgoing_transitions_per_state here.
      }

      state_index current_state=null_state;           	                        assert(current_state + 1 == 0);
      // TODO: This should be combined with another pass through all transitions.
      for(std::vector<transition>::iterator it=m_aut.get_transitions().begin();
                                       it!=m_aut.get_transitions().end(); it++)
      {                                                                         // mCRL2complexity(&m_transitions[std::distance
                                                                                //            (m_aut.get_transitions().begin(), it)], add_work(..., 1), *this);
        const transition& t=*it;                                                    // Because every transition is touched exactly once,
        if (t.to()!=current_state)                                                  // we do not store a physical counter for this.
        {
          for (state_index i=current_state+1; i<=t.to(); ++i)
          {                                                                     // ensure that every state is visited at most once:
                                                                                mCRL2complexity(&m_states[i], add_work(check_complexity::
                                                                                          create_initial_partition__set_start_incoming_transitions, 1), *this);
            m_states[i].start_incoming_transitions=it;
          }
          current_state=t.to();
        }
      }
      for (state_index i=current_state+1; i<m_aut.num_states(); ++i)
      {                                                                         mCRL2complexity(&m_states[i], add_work(check_complexity::
                                                                                          create_initial_partition__set_start_incoming_transitions, 1), *this);
        m_states[i].start_incoming_transitions=m_aut.get_transitions().end();
      }

      // Set the start_same_saC fields in m_outgoing_transitions.
      outgoing_transitions_it_lb it = m_outgoing_transitions.end();
      if (m_outgoing_transitions.begin() < it)
      {
        --it;
        const transition& t=m_aut.get_transitions()[*it->ref_BLC_transitions];
        state_index current_state = t.from();
        label_index current_label = label_or_divergence(t);
        outgoing_transitions_it_lb current_end_same_saC = it;
        while (m_outgoing_transitions.begin() < it)
        {
          --it;                                                                 // mCRL2complexity(&m_transitions[*it->ref_BLC_transitions], add_work(..., 1), *this);
          const transition&t=m_aut.get_transitions()[*it->ref_BLC_transitions];     // Because every transition is touched exactly once,
          const label_index new_label = label_or_divergence(t);                     // we do not store a physical counter for this.
          if (current_state == t.from() && current_label == new_label)
          {
            // We encounter a transition with the same saC.
            // Let it refer to the end.
            it->start_same_saC = current_end_same_saC;
          }
          else
          {
            // We encounter a transition with a different saC.
            current_state = t.from();
            current_label = new_label;
            current_end_same_saC->start_same_saC = std::next(it);
            current_end_same_saC = it;
          }
        }
        current_end_same_saC->start_same_saC = m_outgoing_transitions.begin();
      }                                                                         assert(m_states_in_blocks.size()==m_aut.num_states());
      state_in_block_pointer_lb* lower_i = m_states_in_blocks.data();           assert(initial_block.start_bottom_states==lower_i);
      state_in_block_pointer_lb* upper_i = m_states_in_blocks.data_end();       assert(initial_block.end_states==upper_i);
      for (fixed_vector<state_type_gj_lb>::iterator
                                 i = m_states.begin(); i < m_states.end(); ++i)
      {                                                                         // mCRL2complexity(&m_states[i], add_work(..., 1), *this);
        if (0<i->no_of_outgoing_block_inert_transitions)                            // Because every state is touched exactly once,
        {                                                                           // we do not store a physical counter for this.
          --upper_i;
          upper_i->ref_state=i;
          i->ref_states_in_blocks=upper_i;
        }
        else
        {
          lower_i->ref_state=i;
          i->ref_states_in_blocks=lower_i;
          ++lower_i;
        }
        i->block=&initial_block;
      }                                                                         assert(lower_i == upper_i);
      initial_block.sta.rt_non_bottom_states = lower_i;
      mCRL2log(log::verbose) << "Start refining in the initialisation with super-BLC sets\n";
      for (simple_list<BLC_indicators_lb>::iterator
            blc_it=initial_BLC_source.block_to_constellation.begin();
            initial_BLC_source.block_to_constellation.end()!=blc_it; ++blc_it)
      {                                                                         assert(blc_it->start_same_BLC<blc_it->end_same_BLC);
        BLC_list_iterator it=blc_it->start_same_BLC;                            // mCRL2complexity(blc_it, add_work(...), *this);
        //if (!is_inert_during_init(m_aut.get_transitions()[*it]))                  // Because every BLC set (= set of transitions with the same label)
        //{                                                                         // is touched exactly once, we do not store a physicaal counter for it.
        //  ++no_of_non_constellation_inert_BLC_sets;
        //}
        do
        {
          m_transitions[*it].transitions_per_block_to_constellation=blc_it;
          ++it;
        }
        while (it!=blc_it->end_same_BLC);
      }
//print_data_structures("Just before the first refinements");
      // Algorithm 1, Line 1.3
      for (simple_list<BLC_indicators_lb>::iterator
             blc_it=initial_BLC_source.block_to_constellation.begin();
             initial_BLC_source.block_to_constellation.end()!=blc_it; ++blc_it)
      {                                                                         assert(blc_it->start_same_BLC<blc_it->end_same_BLC);
        if (!is_inert_during_init(m_aut.get_transitions()
                                                    [*blc_it->start_same_BLC]))
        {
          // Algorithm 1, Line 1.4
          refine_super_BLC(*blc_it);
        }
      }
                                                                                assert(check_data_structures("After initial reading before splitting in the initialisation", false));
                                                                                #ifndef NDEBUG
      /* Algorithm 1, line 1.5                                               */   print_data_structures("End initialisation");
                                                                                #endif
                                                                                assert(check_stability("End initialisation"));
      mCRL2log(log::verbose) << "Start stabilizing in the initialisation\n";
                                                                                assert(check_data_structures("End initialisation", false));
      stabilizeB();
    }

    /// \brief Select a block that is not the largest block in a non-trivial constellation.
    /// \returns the index of such a block
    /// \details Either the first or the last block of a constellation is
    /// selected; also, the constellation bounds are adapted accordingly.
    /// However, the caller will have to create a new constellation and set the
    /// block's `constellation` field.
    ///
    /// To ensure the time complexity bounds, it is necessary that the
    /// block returned contains at most 50% of the states in its constellation.
    /// The smaller the better.
    block_type_lb* select_and_remove_a_block_in_a_non_trivial_constellation()
    {                                                                           assert(!m_non_trivial_constellations.empty());
      // Algorithm 1, Line 1.5
      // Do the minimal checking, i.e., only check two blocks in a constellation.
      constellation_type_lb& ci=*m_non_trivial_constellations.back();
      block_type_lb& index_block_B=*ci.start_const_states->ref_state->block; // The first block.
      block_type_lb& second_block_B=
                             *std::prev(ci.end_const_states)->ref_state->block; // The last block.

      if (number_of_states_in_block(index_block_B)<=
                                     number_of_states_in_block(second_block_B))
      {
        ci.start_const_states=index_block_B.end_states;
        return &index_block_B;
      }
      else
      {
        ci.end_const_states=second_block_B.start_bottom_states;
        return &second_block_B;
      }
    }

// =================================================================================================================================
//
//   refine_partition_until_it_becomes_stable.
//
// =================================================================================================================================

    /// \brief number of new bottom states found after constructing the initial partition
    /// \details This count includes all states that were non-bottom state in
    /// the (unstable) trivial partition with a single block.
    state_index no_of_new_bottom_states = 0;

    /// \brief number of non-inert BLC sets in the partition
    /// \details Sets that may contain constellation-inert transitions are not
    /// included in this count (even if that set contains some non-inert
    /// transitions as well).
    //transition_index no_of_non_constellation_inert_BLC_sets = 0;

    void refine_partition_until_it_becomes_stable()
    {
      // This implements the while loop in Algorithm 1 from line 1.4 to 1.19.

      // The instruction below has complexity O(|Act|);
      // calM will contain the m_BLC_transitions slices that need stabilization:
      std::vector<std::pair<BLC_list_iterator, BLC_list_iterator> > calM;
      // Algorithm 1, line 1.6: while (there is a block B that is not a constellation)
      clock_t next_print_time = clock();
      const clock_t rounded_start_time = next_print_time - CLOCKS_PER_SEC/2;
      while (true)
      {
                                                                                #ifndef NDEBUG
                                                                                  print_data_structures("MAIN LOOP");
                                                                                #endif
                                                                                assert(check_data_structures("MAIN LOOP"));
                                                                                assert(check_stability("MAIN LOOP"));
        if (mCRL2logEnabled(log::verbose))
        {
          if (std::clock_t now = std::clock(); next_print_time <= now ||
//true || // for debugging: print the timing and other information every round, not only once a minute.
                                          m_non_trivial_constellations.empty())
          {

            /* -  -  -  -  -print progress information-  -  -  -  - */

            // The formula below should ensure that `next_print_time`
            // increases by a whole number of minutes, so that the
            // progress information is printed every minute (or, if
            // one iteration takes more than one minute, after a whole
            // number of minutes).
            next_print_time+=((now-next_print_time)/(60*CLOCKS_PER_SEC)
                                                    + 1) * (60*CLOCKS_PER_SEC);
            now = (now - rounded_start_time) / CLOCKS_PER_SEC;
            if (0 != now)
            {
              if (60 <= now)
              {
                if (3600 <= now)
                {
                    mCRL2log(log::verbose) << now / 3600 << " h ";
                    now %= 3600;
                }
                mCRL2log(log::verbose) << now / 60 << " min ";
                now %= 60;
              }
              mCRL2log(log::verbose) << now
                              << " sec passed since starting the main loop.\n";
            }
            #define PRINT_SG_PL(counter, sg_string, pl_string) \
                      (counter) << (1 == (counter) ? (sg_string) : (pl_string))
            mCRL2log(log::verbose)
              << (m_non_trivial_constellations.empty()
                                        ? "The reduced LTS contains "
                                        : "The reduced LTS contains at least ")
              << PRINT_SG_PL(no_of_blocks, " state.", " states.");
              // Because we do not have exact BLC sets, we cannot easily count
              // how many transitions the reduced LTS will have.
              //<< (m_non_trivial_constellations.empty() ? " and at least " : " and ")
              //<< PRINT_SG_PL(no_of_non_constellation_inert_BLC_sets,
              //                                " transition.", " transitions.");
            if (1 < no_of_blocks)
            {
              #define PRINT_INT_PERCENTAGE(num,denom) \
                                        (((num) * 200 + (denom)) / (denom) / 2)
              mCRL2log(log::verbose) << " Estimated "
                << PRINT_INT_PERCENTAGE(no_of_constellations - 1,
                                                no_of_blocks - 1)
                << "% done.";
              #undef PRINT_INT_PERCENTAGE
            }
            mCRL2log(log::verbose)
            //  << " Logarithmic estimate: "
            //  << (int)(100.5+std::log((double) no_of_constellations/
            //                      no_of_blocks)
            //                  *log_initial_nr_of_blocks)
            //  << "% done."
                << "\nThe current partition contains ";
            if (m_branching)
            {
              mCRL2log(log::verbose)
                  << PRINT_SG_PL(no_of_new_bottom_states,
                          " new bottom state and ", " new bottom states and ");
            }                                                                   else  {  assert(0==no_of_new_bottom_states);  }
            mCRL2log(log::verbose)
              << PRINT_SG_PL(no_of_constellations,
                     " constellation (of which ", " constellations (of which ")
              << PRINT_SG_PL(m_non_trivial_constellations.size(),
                                  " is nontrivial).\n", " are nontrivial).\n");
            #undef PRINT_SG_PL
          }
        }
        if (m_non_trivial_constellations.empty())
        {
          break;
        }
        // Algorithm 1, line 1.7: Select some small block index_block_B that is not a constellation and its constellation old_constellation
        block_type_lb& index_block_B=
                   *select_and_remove_a_block_in_a_non_trivial_constellation();
        constellation_type_lb& old_constellation=*index_block_B.constellation;

        // Algorithm 1, line 1.8: Move index_block_B to its own constellation new_constellation
        if (old_constellation.start_const_states->ref_state->block==
            std::prev(old_constellation.end_const_states)->ref_state->block)
        {                                                                       assert(m_non_trivial_constellations.back()==&old_constellation);
          // old constellation has become trivial.
          m_non_trivial_constellations.pop_back();
        }
        constellation_type_lb& new_constellation = *
                #ifdef USE_POOL_ALLOCATOR
                    simple_list<BLC_indicators_lb>::get_pool().
                    template construct<constellation_type_lb>
                #else
                    new constellation_type_lb
                #endif
                                           (index_block_B.start_bottom_states,
                                                     index_block_B.end_states);
        ++no_of_constellations;
//std::cerr << "Moving " << index_block_B.debug_id(*this) << " from "
//<< old_constellation.debug_id(*this) << " to a new "
//<< new_constellation.debug_id(*this) << ".\n";
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
        /* Block index_block_B is moved to the new constellation but we shall*/   // new_constellation.work_counter=old_constellation.work_counter;
        /* not yet assign                                                    */   unsigned char const max_C=check_complexity::log_n-check_complexity::
        /* index_block_B.constellation=&new_constellation;                   */                    ilog2(number_of_states_in_constellation(new_constellation));
        /* because we need the old constellation pointer in                  */   mCRL2complexity(&index_block_B, add_work(check_complexity::
        /* update_the_doubly_linked_list_LBC_new_constellation().            */        refine_partition_until_it_becomes_stable__find_splitter, max_C), *this);
                                                                                #endif
        // Here the variables block.to_constellation and the doubly linked list
        // L_B->C in blocks must be still be updated.
        // This happens further below.

        // Algorithm 1, Line 1.9: Maintain data structures (to point to new_constellation properly)
        // We need to walk through all incoming transitions twice: first to
        // update the saC references provisionally (make them at least point at
        // each other). In the second run we can update them to their final
        // values. (If we would do this in a single run, we might do work
        // quadratic in the number of transitions, because every time we find a
        // transition to the new constellation, we would need to update the
        // start_same_saC pointers for every transition that has been found
        // earlier.)
        // walk through all states in index_block_B
        for (state_in_block_pointer_lb* i=index_block_B.start_bottom_states;
                                              i!=index_block_B.end_states; ++i)
        {                                                                       // mCRL2complexity(m_states[*i], add_work(..., max_C), *this);
          // and visit the incoming transitions.                                    // subsumed under the above counter
          const std::vector<transition>::iterator end_it=
                   (std::next(i->ref_state)==m_states.end())
                         ? m_aut.get_transitions().end()
                         : std::next(i->ref_state)->start_incoming_transitions;
          for(std::vector<transition>::iterator
                    j=i->ref_state->start_incoming_transitions; j!=end_it; ++j)
          {
            const transition& t=*j;
            const transition_index t_index=
                             std::distance(m_aut.get_transitions().begin(), j);
            // Update the state-action-constellation (saC) references in        // mCRL2complexity(&m_transitions[t_index], add_work(..., max_C), *this);
            // m_outgoing_transitions.                                              // subsumed under the above counter
            const outgoing_transitions_it_lb old_pos=
                               m_transitions[t_index].ref_outgoing_transitions;
            const outgoing_transitions_it_lb
                      end_same_saC = old_pos->start_same_saC<old_pos
                                           ? old_pos : old_pos->start_same_saC;
            const outgoing_transitions_it_lb
                                          new_pos=end_same_saC->start_same_saC; assert(m_states[t.from()].start_outgoing_transitions<=new_pos);
            if (old_pos != new_pos)
            {                                                                   assert(new_pos<old_pos);
              std::swap(old_pos->ref_BLC_transitions,
                        new_pos->ref_BLC_transitions);
              m_transitions[*old_pos->ref_BLC_transitions].
                                              ref_outgoing_transitions=old_pos;
              m_transitions[*new_pos->ref_BLC_transitions].
                                              ref_outgoing_transitions=new_pos;
            }                                                                   assert(new_pos<=end_same_saC);
            end_same_saC->start_same_saC = std::next(new_pos);
            // correct start_same_saC provisionally: make them at least point
            // at each other.  In the new saC-slice, all transitions point to
            // the first one, except the first one: that shall point at the
            // last one.
            new_pos->start_same_saC = new_pos;
            if (m_states[t.from()].start_outgoing_transitions<new_pos)
            {
              // Check if t is the first transition in the new saC slice:
              const transition& prev_t = m_aut.get_transitions()
                                    [*std::prev(new_pos)->ref_BLC_transitions]; assert(prev_t.from() == t.from());
              if (m_states[prev_t.to()].block == &index_block_B &&
                  label_or_divergence(prev_t) == label_or_divergence(t))
              {
                // prev_t also belongs to the new saC slice.
                new_pos->start_same_saC = std::prev(new_pos)->start_same_saC;   assert(m_states[t.from()].start_outgoing_transitions<=new_pos->start_same_saC);
                                                                                assert(new_pos->start_same_saC<new_pos);
                                                                                assert(std::prev(new_pos)==new_pos->start_same_saC->start_same_saC);
                new_pos->start_same_saC->start_same_saC = new_pos;
              }
            }
          }
        }
        calM.clear();

        // Walk through all states in index_block_B
        for (state_in_block_pointer_lb* i=index_block_B.start_bottom_states;
                                              i!=index_block_B.end_states; ++i)
        {                                                                       // mCRL2complexity(m_states[*i], add_work(..., max_C), *this);
          // and visit the incoming transitions.                                    // subsumed under the above counter
//std::cerr << "Handling incoming transitions of " << i->ref_state->debug_id(*this) << ":\n";
          const std::vector<transition>::iterator end_it=
                  (std::next(i->ref_state)==m_states.end())
                         ? m_aut.get_transitions().end()
                         : std::next(i->ref_state)->start_incoming_transitions;
          for(std::vector<transition>::iterator
                    j=i->ref_state->start_incoming_transitions; j!=end_it; ++j)
          {
            const transition& t=*j;
            const transition_index t_index=
                             std::distance(m_aut.get_transitions().begin(), j); assert(m_states[t.to()].block == &index_block_B);
            //bool source_block_is_singleton=
            //       (1>=number_of_states_in_block(*m_states[t.from()].block)); // mCRL2complexity(&m_transitions[t_index], add_work(..., max_C), *this);
                                                                                    // subsumed under the above counter
            // Give the saC slice of this transition its final correction
            const outgoing_transitions_it_lb out_pos=
                               m_transitions[t_index].ref_outgoing_transitions;
            const outgoing_transitions_it_lb start_new_saC=
                                                       out_pos->start_same_saC;
            if (start_new_saC < out_pos)
            {
              // not the first transition in the saC-slice
              if (out_pos < start_new_saC->start_same_saC)
              {
                // not the last transition in the saC-slice
                // so make its start_same_saC point at the last transition in
                // the saC-slice
                out_pos->start_same_saC = start_new_saC->start_same_saC;
              }
            }

            // Update the doubly linked list L_B->C in blocks as
            // old_constellation is split in new_constellation (containing
            // index_block_B) and old_constellation \ index_block_B.
            if (update_the_doubly_linked_list_LBC_new_constellation
                                                   (index_block_B, t, t_index))
            {
              // a new BLC set has been constructed, insert its start position
              // into calM (even if its source block is a singleton or it seems
              // to contain inert transitions---the new BLC set may have other
              // source blocks that are not singletons and are not a subset of
              // the new constellation).
              BLC_list_iterator BLC_pos=m_transitions[t_index].
                                 ref_outgoing_transitions->ref_BLC_transitions; assert(t_index == *BLC_pos);
              // Algorithm 1, Line 1.10: add this transition (and those in the same BLC set) to calM
              calM.emplace_back(BLC_pos, BLC_pos);
              // The end-position (the second element in the pair) will need to be corrected later.
            }
          }
        }
        index_block_B.constellation=&new_constellation;

//std::cerr << "1.10\n";
        // Algorithm 1, Line 1.10: add the transitions in the BLC set to calM
        // correct the end-positions of calM entries
        if (!calM.empty())
        {
          for (std::vector<std::pair<BLC_list_iterator, BLC_list_iterator> >::
                                             iterator calM_elt=calM.begin();; )
          {
            simple_list <BLC_indicators_lb>::iterator
                  ind=m_transitions[*calM_elt->first].
                                        transitions_per_block_to_constellation; mCRL2complexity(ind, add_work(check_complexity::
            /* Algorithm 1, Line 1.17                                        */    refine_partition_until_it_becomes_stable__correct_end_of_calM,max_C),*this);
            /* check if all transitions were moved to the new constellation, */ assert(ind->start_same_BLC==calM_elt->first);
            /* or some transitions to the old constellation have remained:   */ assert(!ind->has_marked_transitions());
            const transition& last_t=
                        m_aut.get_transitions()[*std::prev(ind->end_same_BLC)]; assert(m_states[last_t.to()].block->constellation==&new_constellation);
                                                                                assert(ind->start_same_BLC<ind->end_same_BLC);
            const transition* next_t=nullptr;
            if (is_inert_during_init(last_t) || // inert transitions may have become non-inert right now
                (ind->end_same_BLC<m_BLC_transitions.data_end() &&
                 (next_t=&m_aut.get_transitions()[*ind->end_same_BLC],
                  m_states[last_t.from()].block->block_BLC_source==
                            m_states[next_t->from()].block->block_BLC_source &&
                  label_or_divergence(last_t)==label_or_divergence(*next_t) &&
                  &old_constellation==
                                 m_states[next_t->to()].block->constellation)))
            {
              // there are some transitions to the corresponding co-splitter,
              // so we will have to stabilize the block
              calM_elt->second = ind->end_same_BLC;
              ++calM_elt;
              if (calM_elt==calM.end())
              {
                break;
              }
            }
            else
            {
              // all transitions in the old BLC set have moved to the new BLC
              // set; as the old BLC set was stable, so is the new one.
              // We can skip this element.
              if (std::next(calM_elt)==calM.end())
              {
                // to avoid protests by the MSVC compiler we have to do this
                // check beforehand (if calM_elt points to the last element of
                // the vector, the standard mandates that the iterator becomes
                // invalid.)
                calM.pop_back();
                break;
              }
              else
              {
                calM_elt->first=calM.back().first;
                calM.pop_back();
              }
            }
          }
        }

        // ---------------------------------------------------------------------------------------------
        // First carry out a co-split of index_block_B with respect to old_constellation and an action tau.
//std::cerr << "1.11\n";
        // Algorithm 1, Line 1.11: if |index_block_B| > 1
        if (m_branching)
        {
          // Here we should check whether we need to do
          // ++no_of_non_constellation_inert_BLC_sets;
          // but I do not know how to test for that easily.
          // (One would need to find out whether there is a relevant BLC set
          // that contains tau-transitions from index_block_B to
          // old_constellation.  This can only be done by going through the
          // outgoing transitions of index_block_B, even when it has only one
          // state.  Checking whether these sets overlap is easy.)
          if (1<number_of_states_in_block(index_block_B))
          {
            block_that_needs_refinement_type co_refinement_info(index_block_B);
            // go through all outgoing tau-transitions of index_block_B
            for(state_in_block_pointer_lb* i=index_block_B.start_bottom_states;
                                              i!=index_block_B.end_states; ++i)
            {                                                                   // mCRL2complexity(m_states[*i], add_work(..., max_C), *this);
                                                                                    // subsumed under the above counter refine_partition_until_it_becomes_stable__find_splitter
              const outgoing_transitions_it_lb end_it=
                    (std::next(i->ref_state)==m_states.end())
                         ? m_outgoing_transitions.end()
                         : std::next(i->ref_state)->start_outgoing_transitions;
              for(outgoing_transitions_it_lb
                    j=i->ref_state->start_outgoing_transitions; j!=end_it; ++j)
              {
                const transition& tr=m_aut.get_transitions()
                                                     [*j->ref_BLC_transitions]; assert(&m_states[tr.from()]==&*i->ref_state);
                                                                                // mCRL2complexity(m_transitions[*j->ref_BLC_transitions], ...);
                if (!m_aut.is_tau(m_aut_apply_hidden_label_map(tr.label())))        // subsumed under the above counter
                {
                                                                                #ifndef NDEBUG
                  /* The tau-transitions are all at the beginning of the     */   while (++j != end_it) { assert(!m_aut.is_tau(m_aut_apply_hidden_label_map
                  /* outgoing transitions.                                   */                 (m_aut.get_transitions()[*j->ref_BLC_transitions].label()))); }
                                                                                #endif
                  break;
                }
                if (is_inert_during_init_if_branching(tr) &&
                    m_states[tr.to()].block->constellation==&old_constellation)
                {
                  // This is a transition that has just become non-C-inert.
                  if (i<index_block_B.sta.rt_non_bottom_states)
                  {
                    if (i->ref_state->ref_states_in_blocks >=
                            co_refinement_info.start_bottom_states[ReachAlw+1])
                    {                                                           static_assert(ReachAlw + 1 == AvoidSml);
                      // Move the state to ReachAlw
                      swap_states_in_states_in_block
                           (co_refinement_info.start_bottom_states[ReachAlw+1],
                                           i->ref_state->ref_states_in_blocks);
                      ++co_refinement_info.start_bottom_states[ReachAlw+1];
                    }
                  }
                  else
                  {
                    if (undefined==i->ref_state->counter)
                    {
                      // Add the state to pot-ReachAlw
                      i->ref_state->counter = marked(ReachAlw)+
                          i->ref_state->no_of_outgoing_block_inert_transitions; assert(is_in_marked_range_of(i->ref_state->counter, ReachAlw));
                      co_refinement_info.potential_non_bottom_states
                                                      [ReachAlw].push_back(*i);
                    }
                  }
                }
              }
            }
            // Algorithm 1, Line 1.16: if B.ReachAlw union B.pot-ReachAlw is not empty then
            // (additionally we test whether there are bottom states in AvoidSml.
            // Otherwise the split is trivial.)
            if (0!=co_refinement_info.bottom_size(AvoidSml))
            {
              if  (0!=co_refinement_info.bottom_size(ReachAlw) ||
                   !co_refinement_info.
                                potential_non_bottom_states[ReachAlw].empty())
              {
                // Algorithm 1, Line 1.17: Split index_block_B under the
                // transitions that have become non-C-inert.
                // The tau co-splitter contains transitions that have just become
                // non-inert.
                four_way_splitB(co_refinement_info,
                                       &old_constellation, &new_constellation);
              }
            }
            else
            {
              clear_state_counters(co_refinement_info.
                                 potential_non_bottom_states[ReachAlw].begin(),
                co_refinement_info.potential_non_bottom_states[ReachAlw].end(),
                                                                index_block_B);
            }
            index_block_B.refinement_info = nullptr;
          }
        }
        // Algorithm 1, Line 1.18: while calM is not empty do
        for (std::pair<BLC_list_iterator, BLC_list_iterator> calM_elt: calM)
        {                                                                       // mCRL2complexity(..., add_work(..., max_C), *this);
                                                                                    // not needed as the inner loop is always executed at least once.
                                                                                #ifndef NDEBUG
                                                                                  print_data_structures("Main loop");
                                                                                #endif
                                                                                assert(check_stability("Main loop", &calM, &calM_elt, &old_constellation, &new_constellation));
                                                                                assert(check_data_structures("Main loop", false));
          /* Algorithm 1, Line 1.19: Pick some super-BLC set SmallSp in calM */ assert(calM_elt.first < calM_elt.second);
          do
          {
            simple_list<BLC_indicators_lb>::iterator
                    small_splitter=m_transitions[*std::prev(calM_elt.second)].
                                        transitions_per_block_to_constellation; mCRL2complexity(small_splitter, add_work(check_complexity::
                                                                                    refine_partition_until_it_becomes_stable__execute_main_split,max_C),*this);
            /* Algorithm 1, Line 1.20: Remove SmallSp from calM              */ assert(small_splitter->end_same_BLC==calM_elt.second);
                                                                                assert(small_splitter->is_stable());
            calM_elt.second = small_splitter->start_same_BLC;                   assert(small_splitter->start_same_BLC<small_splitter->end_same_BLC);

            const transition& first_t=
                      m_aut.get_transitions()[*small_splitter->start_same_BLC]; assert(m_states[first_t.to()].block->constellation==&new_constellation);
            const BLC_source_type& block_BLC_source =
                             *m_states[first_t.from()].block->block_BLC_source;
            if (is_inert_during_init(first_t) &&
                new_constellation.start_const_states <=
                                           block_BLC_source.start_BLC_source &&
                block_BLC_source.end_BLC_source <=
                                            new_constellation.end_const_states)
            {
              // The BLC set only contains transitions that are still
              // constellation-inert. It will not lead to any refinements.
              continue;
            }
            // Algorithm 1, Line 1.22: Call refine_super_BLC()
            simple_list<BLC_indicators_lb>::iterator const large_splitter =
                  block_BLC_source.block_to_constellation.prev(small_splitter);
            const transition* large_t;
            if(block_BLC_source.block_to_constellation.end()==large_splitter ||
               large_splitter->start_same_BLC==large_splitter->end_same_BLC ||
               (large_t =
                     &m_aut.get_transitions()[*large_splitter->start_same_BLC], assert(m_states[large_t->from()].block->block_BLC_source==&block_BLC_source),
                m_states[large_t->to()].block->constellation !=
                                                         &old_constellation) ||
               label_or_divergence(first_t) != label_or_divergence(*large_t))
            {
              // The large splitter is empty.
              if (is_inert_during_init(first_t) &&
                  block_BLC_source.start_BLC_source <
                                          old_constellation.end_const_states &&
                  old_constellation.start_const_states <
                                               block_BLC_source.end_BLC_source)
              {
                // But the small splitter may contain transitions that have
                // just become non-constellation-inert, so we still have to
                // split.
                refine_super_BLC(*small_splitter, nullptr);
              }
            }
            else
            {
              refine_super_BLC(*small_splitter, &*large_splitter);
            }
          }
          while (calM_elt.first < calM_elt.second);
        }
                                                                                #ifndef NDEBUG
                                                                                  print_data_structures("Before stabilize");
                                                                                #endif
                                                                                assert(check_data_structures("Before stabilize", false));
        /* Algorithm 1, Line 1.19                                            */ assert(check_stability("Before stabilize"));
        stabilizeB();
      }
                                                                                #if !defined(NDEBUG) || defined(COUNT_WORK_BALANCE)
                                                                                  check_complexity::print_grand_totals();
                                                                                #endif
    }

  public:
    /// time measurement after creating the initial partition (but before the first call to `stabilizeB()`)
    std::clock_t end_initial_part;

    /// \brief constructor
    /// \details The constructor constructs the data structures and immediately
    /// calculates the partition corresponding with the bisimulation quotient.
    /// It does not adapt the LTS to represent the quotient's transitions.
    /// It is assumed that there are no tau-loops in aut.
    /// \param aut                 LTS that needs to be reduced
    /// \param branching           If true branching bisimulation is used,
    ///                            otherwise strong bisimulation is
    ///                            applied.
    /// \param preserve_divergence If true and branching is true, preserve
    ///                            tau loops on states.
    bisim_partitioner_gj_lazy_BLC(LTS_TYPE& aut, const bool branching = false, const bool preserve_divergence = false)
        : m_aut(aut),
          m_states(aut.num_states()),
          m_outgoing_transitions(aut.num_transitions()),
          m_transitions(aut.num_transitions()),
          m_states_in_blocks(aut.num_states()),

          m_BLC_transitions(aut.num_transitions()),
          m_branching(branching),
          m_preserve_divergence(preserve_divergence)
    {                                                                           assert(m_branching || !m_preserve_divergence);
//log::logger::set_reporting_level(log::debug);
      mCRL2log(log::debug) << "Start initialisation.\n";
      // Apply the hidden labels explicitly as the information about hidden labels is not used.
      aut.rename_hidden_labels_to_tau(); 
      create_initial_partition();
      end_initial_part=std::clock();
      mCRL2log(log::debug) << "After initialisation there are "
              << no_of_blocks << " equivalence classes. Start refining. \n";
      refine_partition_until_it_becomes_stable();                               assert(check_data_structures("READY"));
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
void bisimulation_reduce_gj_lazy_BLC(LTS_TYPE& l, const bool branching = false,
                                         const bool preserve_divergence = false)
{
    if (1 >= l.num_states())
    {
        mCRL2log(log::warning) << "There is only 1 state in the LTS. It is not "
                "guaranteed that branching bisimulation minimisation runs in "
                "time O(m log n).\n";
    }
    // Algorithm 1, Line 1.1: Find tau-SCCs and contract each of them to a
    // single state
    const std::clock_t start_SCC=std::clock();
    mCRL2log(log::verbose) << "Start SCC\n";
    if (branching)
    {
        scc_reduce(l, preserve_divergence);
    }

    // Now apply the branching bisimulation reduction algorithm.  If there
    // are no taus, this will automatically yield strong bisimulation.
    const std::clock_t start_part=std::clock();
    mCRL2log(log::debug) << "Start Partitioning\n";
    bisim_partitioner_gj_lazy_BLC<LTS_TYPE> bisim_part(l,
                                               branching, preserve_divergence);

    // Assign the reduced LTS
    const std::clock_t end_part=std::clock();
    mCRL2log(log::debug) << "Start finalizing\n";
    bisim_part.finalize_minimized_LTS();

//log::logger::set_reporting_level(log::debug);
    if (mCRL2logEnabled(log::debug))
    {
        const std::clock_t end_finalizing=std::clock();
        const int prec=static_cast<int>
                                 (std::log10(CLOCKS_PER_SEC)+0.69897000433602);
            // For example, if CLOCKS_PER_SEC>=     20: >=2 digits
            //              If CLOCKS_PER_SEC>=    200: >=3 digits
            //              If CLOCKS_PER_SEC>=2000000: >=7 digits

        double runtime[5];
        runtime[0]=(double) (end_finalizing                        -                        start_SCC)/CLOCKS_PER_SEC; // total time
        runtime[1]=(double) (                                                    start_part-start_SCC)/CLOCKS_PER_SEC;
        runtime[2]=(double) (                        bisim_part.end_initial_part-start_part          )/CLOCKS_PER_SEC;
        runtime[3]=(double) (               end_part-bisim_part.end_initial_part                     )/CLOCKS_PER_SEC;
        runtime[4]=(double) (end_finalizing-end_part                                                 )/CLOCKS_PER_SEC;
        if (runtime[0]>=60.0)
        {
            int min[sizeof(runtime)/sizeof(runtime[0])];
            for (unsigned i = 0; i < sizeof(runtime)/sizeof(runtime[0]); ++i)
            {
                min[i] = static_cast<int>(runtime[i]) / 60;
                runtime[i] -= 60 * min[i];
            }
            if (min[0]>=60)
            {
                int h[sizeof(runtime)/sizeof(runtime[0])];
                for (unsigned i=0; i < sizeof(runtime)/sizeof(runtime[0]); ++i)
                {
                    h[i] = min[i] / 60;
                    min[i] %= 60;
                }
                int width = static_cast<int>(std::log10(h[0])) + 1;

                mCRL2log(log::debug) << std::fixed << std::setprecision(prec)
                    << "Time spent on contracting SCCs: " << std::setw(width) << h[1] << "h " << std::setw(2) << min[1] << "min " << std::setw(prec+3) << runtime[1] << "s\n"
                       "Time spent on initial partition:" << std::setw(width) << h[2] << "h " << std::setw(2) << min[2] << "min " << std::setw(prec+3) << runtime[2] << "s\n"
                       "Time spent on stabilize+refine: " << std::setw(width) << h[3] << "h " << std::setw(2) << min[3] << "min " << std::setw(prec+3) << runtime[3] << "s\n"
                       "Time spent on finalizing:       " << std::setw(width) << h[4] << "h " << std::setw(2) << min[4] << "min " << std::setw(prec+3) << runtime[4] << "s\n"
                       "Total CPU time:                 " << std::setw(width) << h[0] << "h " << std::setw(2) << min[0] << "min " << std::setw(prec+3) << runtime[0] << "s\n"
                       "BENCHMARK TIME: " << static_cast<double>(end_part-start_part)/CLOCKS_PER_SEC << "\n"
                    << std::defaultfloat;
            }
            else
            {
                mCRL2log(log::debug) << std::fixed << std::setprecision(prec)
                    << "Time spent on contracting SCCs: " << std::setw(2) << min[1] << "min " << std::setw(prec+3) << runtime[1] << "s\n"
                       "Time spent on initial partition:" << std::setw(2) << min[2] << "min " << std::setw(prec+3) << runtime[2] << "s\n"
                       "Time spent on stabilize+refine: " << std::setw(2) << min[3] << "min " << std::setw(prec+3) << runtime[3] << "s\n"
                       "Time spent on finalizing:       " << std::setw(2) << min[4] << "min " << std::setw(prec+3) << runtime[4] << "s\n"
                       "Total CPU time:                 " << std::setw(2) << min[0] << "min " << std::setw(prec+3) << runtime[0] << "s\n"
                       "BENCHMARK TIME: " << static_cast<double>(end_part-start_part)/CLOCKS_PER_SEC << "\n"
                    << std::defaultfloat;
            }
        }
        else
        {
            mCRL2log(log::debug) << std::fixed << std::setprecision(prec)
                << "Time spent on contracting SCCs: " << std::setw(prec+3) << runtime[1] << "s\n"
                   "Time spent on initial partition:" << std::setw(prec+3) << runtime[2] << "s\n"
                   "Time spent on stabilize+refine: " << std::setw(prec+3) << runtime[3] << "s\n"
                   "Time spent on finalizing:       " << std::setw(prec+3) << runtime[4] << "s\n"
                   "Total CPU time:                 " << std::setw(prec+3) << runtime[0] << "s\n"
                   "BENCHMARK TIME: " << static_cast<double>(end_part-start_part)/CLOCKS_PER_SEC << "\n"
                << std::defaultfloat;
        }
    }
}


/// \brief Checks whether the two initial states of two LTSs are strong or
/// (divergence-preserving) branching bisimilar.
/// \details This routine uses the experimental O(m log n) branching
/// bisimulation algorithm developed in 2025 by Jan Friso Groote and David N.
/// Jansen.  It runs in O(m log n) time and uses O(m) memory, where n is the
/// number of states and m is the number of transitions.
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
bool destructive_bisimulation_compare_gj_lazy_BLC(LTS_TYPE& l1, LTS_TYPE& l2,
        const bool branching = false, const bool preserve_divergence = false,
        const bool generate_counter_examples = false,
        const std::string& /*counter_example_file*/ = "",
        bool /*structured_output*/ = false)
{
    if (generate_counter_examples)
    {
        mCRL2log(log::warning) << "The GJ25 branching bisimulation "
                              "algorithm does not generate counterexamples.\n";
    }
    std::size_t init_l2(l2.initial_state() + l1.num_states());
    detail::merge(l1, std::move(l2));
    l2.clear(); // No use for l2 anymore.

    if (branching)
    {
        detail::scc_partitioner<LTS_TYPE> scc_part(l1);
        scc_part.replace_transition_system(preserve_divergence);
        init_l2 = scc_part.get_eq_class(init_l2);
    }                                                                           else  {  assert(!preserve_divergence);  }
                                                                                assert(1 < l1.num_states());
    bisim_partitioner_gj_lazy_BLC<LTS_TYPE> bisim_part(l1,
                                               branching, preserve_divergence);

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
inline bool bisimulation_compare_gj_lazy_BLC(const LTS_TYPE& l1,
                                        const LTS_TYPE& l2,
                                        const bool branching = false,
                                        const bool preserve_divergence = false)
{
    LTS_TYPE l1_copy(l1);
    LTS_TYPE l2_copy(l2);
    return destructive_bisimulation_compare_gj_lazy_BLC(l1_copy, l2_copy,
                                               branching, preserve_divergence);
}


} // end namespace detail
// end namespace lts
// end namespace mcrl2

#endif // ifndef LIBLTS_BISIM_GJ_LAZY_BLC_H
