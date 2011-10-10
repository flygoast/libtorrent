#include "config.h"

#include "net/address_list.h"

#include "tracker_list_test.h"

namespace std { using namespace tr1; }

CPPUNIT_TEST_SUITE_REGISTRATION(tracker_list_test);

#define TRACKER_SETUP()                                                 \
  torrent::TrackerList tracker_list;                                    \
  int success_counter = 0;                                              \
  int failure_counter = 0;                                              \
  tracker_list.set_slot_success(std::bind(&increment_value, &success_counter)); \
  tracker_list.set_slot_failed(std::bind(&increment_value, &failure_counter));

#define TRACKER_INSERT(group, name)                             \
  TrackerTest* name = new TrackerTest(&tracker_list, "");       \
  tracker_list.insert(group, name);

void increment_value(int* value) { (*value)++; }

bool
TrackerTest::trigger_success() {
  torrent::TrackerList::address_list address_list;

  return trigger_success(&address_list);
}

bool
TrackerTest::trigger_success(torrent::TrackerList::address_list* address_list) {
  if (parent() == NULL || !is_busy() || !is_open())
    return false;

  m_busy = false;
  m_open = !m_close_on_done;
  parent()->receive_success(this, address_list);

  m_requesting_state = 0;
  return true;
}

bool
TrackerTest::trigger_failure() {
  if (parent() == NULL || !is_busy() || !is_open())
    return false;

  m_busy = false;
  m_open = !m_close_on_done;
  parent()->receive_failed(this, "failed");
  m_requesting_state = 0;
  return true;
}

void
tracker_list_test::test_basic() {
  TRACKER_SETUP();
  TRACKER_INSERT(0, tracker_0);

  CPPUNIT_ASSERT(tracker_0 == tracker_list[0]);

  CPPUNIT_ASSERT(tracker_list[0]->parent() == &tracker_list);
  CPPUNIT_ASSERT(std::distance(tracker_list.begin_group(0), tracker_list.end_group(0)) == 1);
  CPPUNIT_ASSERT(tracker_list.find_usable(tracker_list.begin()) != tracker_list.end());
}

// Test clear.

void
tracker_list_test::test_single_success() {
  TRACKER_SETUP();
  TRACKER_INSERT(0, tracker_0);

  CPPUNIT_ASSERT(!tracker_0->is_busy());
  CPPUNIT_ASSERT(!tracker_0->is_open());
  CPPUNIT_ASSERT(tracker_0->requesting_state() == -1);

  tracker_list.send_state(1);

  CPPUNIT_ASSERT(tracker_0->is_busy());
  CPPUNIT_ASSERT(tracker_0->is_open());
  CPPUNIT_ASSERT(tracker_0->requesting_state() == 1);

  CPPUNIT_ASSERT(tracker_0->trigger_success());

  CPPUNIT_ASSERT(!tracker_0->is_busy());
  CPPUNIT_ASSERT(!tracker_0->is_open());
  CPPUNIT_ASSERT(tracker_0->requesting_state() == 0);
  
  CPPUNIT_ASSERT(success_counter == 1 && failure_counter == 0);
  CPPUNIT_ASSERT(tracker_0->success_counter() == 1);
  CPPUNIT_ASSERT(tracker_0->failed_counter() == 0);
}

void
tracker_list_test::test_single_failure() {
  TRACKER_SETUP();
  TRACKER_INSERT(0, tracker_0);

  tracker_list.send_state(1);
  CPPUNIT_ASSERT(tracker_0->trigger_failure());

  CPPUNIT_ASSERT(!tracker_0->is_busy());
  CPPUNIT_ASSERT(!tracker_0->is_open());
  CPPUNIT_ASSERT(tracker_0->requesting_state() == 0);

  CPPUNIT_ASSERT(success_counter == 0 && failure_counter == 1);
  CPPUNIT_ASSERT(tracker_0->success_counter() == 0);
  CPPUNIT_ASSERT(tracker_0->failed_counter() == 1);

  tracker_list.send_state(1);
  CPPUNIT_ASSERT(tracker_0->trigger_success());

  CPPUNIT_ASSERT(success_counter == 1 && failure_counter == 1);
  CPPUNIT_ASSERT(tracker_0->success_counter() == 1);
  CPPUNIT_ASSERT(tracker_0->failed_counter() == 0);
}

void
tracker_list_test::test_single_closing() {
  TRACKER_SETUP();
  TRACKER_INSERT(0, tracker_0);

  CPPUNIT_ASSERT(!tracker_0->is_open());

  tracker_0->set_close_on_done(false);
  tracker_list.send_state(1);

  CPPUNIT_ASSERT(tracker_0->is_open());
  CPPUNIT_ASSERT(tracker_0->trigger_success());

  CPPUNIT_ASSERT(!tracker_0->is_busy());
  CPPUNIT_ASSERT(tracker_0->is_open());

  tracker_list.close_all();

  CPPUNIT_ASSERT(!tracker_0->is_open());
}

// test last_connect timer.
