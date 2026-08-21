#include <gtest/gtest.h>

#include "utils.h"

TEST(BitReverseTest, ReversesEightBits) {
    EXPECT_EQ(reverse_bits_n(0b00000001, 8), 0b10000000);
    EXPECT_EQ(reverse_bits_n(0b00000010, 8), 0b01000000);
    EXPECT_EQ(reverse_bits_n(0b10000000, 8), 0b00000001);
    EXPECT_EQ(reverse_bits_n(0b11000100, 8), 0b00100011);
}

TEST(BitReverseTest, ReversesFourBits) {
    EXPECT_EQ(reverse_bits_n(0b0001, 4), 0b1000);
    EXPECT_EQ(reverse_bits_n(0b0011, 4), 0b1100);
    EXPECT_EQ(reverse_bits_n(0b1010, 4), 0b0101);
}

TEST(BitReverseTest, ReversesOddNumberOfBits) {
    EXPECT_EQ(reverse_bits_n(0b001, 3), 0b100);
    EXPECT_EQ(reverse_bits_n(0b010, 3), 0b010);
    EXPECT_EQ(reverse_bits_n(0b101, 3), 0b101);
}

TEST(BitReverseTest, ZeroRemainsZero) {
    EXPECT_EQ(reverse_bits_n(0, 8), 0);
}

TEST(BitReverseTest, ReversesThreeBits) {
    EXPECT_EQ(reverse_bits_n(0b001, 3), 0b100);
    EXPECT_EQ(reverse_bits_n(0b010, 3), 0b010);
    EXPECT_EQ(reverse_bits_n(0b100, 3), 0b001);

    EXPECT_EQ(reverse_bits_n(0b011, 3), 0b110);
    EXPECT_EQ(reverse_bits_n(0b101, 3), 0b101);
    EXPECT_EQ(reverse_bits_n(0b110, 3), 0b011);
}

TEST(BitReverseTest, ReversesFiveBits) {
    EXPECT_EQ(reverse_bits_n(0b00001, 5), 0b10000);
    EXPECT_EQ(reverse_bits_n(0b00100, 5), 0b00100);
    EXPECT_EQ(reverse_bits_n(0b10000, 5), 0b00001);

    EXPECT_EQ(reverse_bits_n(0b10110, 5), 0b01101);
}

TEST(BitReverseTest, ReversesOneBit) {
    EXPECT_EQ(reverse_bits_n(0b0, 1), 0b0);
    EXPECT_EQ(reverse_bits_n(0b1, 1), 0b1);
}
