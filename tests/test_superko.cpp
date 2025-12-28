#include "gtest/gtest.h"
#include "board.h"

TEST(SuperkoTest, PreventRepeatState) {
  Board b(5);
  // Create position A: a single black stone at (0,0)
  b.set(0,0,BLACK);
  uint64_t hashA = b.zobrist();
  // record position A in history explicitly
  // (simulate that A occurred previously)
  // Note: Normally board constructor already pushed initial empty hash.
  // Append A to history by simulating: set current grid hash and history.
  // We don't have a public API to add to history, but we can simulate by creating a new Board and copying values.

  // For test, make a fresh board to represent current board at B (empty)
  Board b2(5);
  // simulate that history contains A then B
  // Workaround: set b2's grid to empty (it already is) and set history by placing then reverting using internals is not public.
  // Simpler approach: place on b2 the moves to reach B then manually ensure that placing a stone at (0,0) would recreate A.

  // We'll manually push by using place: first, on b place we already have A; now clear b to empty and push history entries via place/undo is not available.
  // Simpler: Use the API as designed: start with empty board b2 (history has empty). Now place at (0,0) should be allowed unless it repeats history.

  // To create repeat detection, we will manually manipulate by constructing b3 that has had A before in history.
  Board b3(5);
  // simulate that board previously saw A: push A into history by using b3's place to make A then reset to empty by reinitializing and copying history.
  EXPECT_TRUE(b3.place(0,0,BLACK)); // now b3 has A in history
  // Now create new board B (empty) and copy history from b3 to simulate that A occurred before
  Board b4(5);
  // Copy history by replaying: We cannot access internals to set history; but we can simulate by making b4 follow b3 then clear
  // Undo not available, so we can instead check that after b3 placed and then we clear the board by creating a new board, attempting to place at (0,0) should be rejected if history were shared.
  // To make test deterministic, let's operate on b3 directly: after placing at (0,0) (A), now we attempt to remove the stone by a capture move by opponent, creating B, then try to retake A and expect false.

  // Setup capture to remove the black at (0,0)
  // Surround (0,0) with white stones so a white move captures it
  EXPECT_TRUE(b3.place(1,0,WHITE));
  EXPECT_TRUE(b3.place(0,1,WHITE));
  // Now black at (0,0) has no liberties and should be captured when white plays a capturing move if rules allow; but current place mechanics don't auto-capture unless move causes it.
  // Instead, directly check that after this white surrounding, black is removed when white plays at (1,1)
  EXPECT_TRUE(b3.place(1,1,WHITE));
  // After these moves, black at (0,0) should have been captured
  EXPECT_EQ(b3.get(0,0), EMPTY);

  // Now current state is B (empty at 0,0). Attempting to place BLACK at (0,0) would recreate previous A (which is in history), so should be rejected by superko
  EXPECT_FALSE(b3.place(0,0,BLACK));
}
