#include <gtest/gtest.h>

#include "ac_state.h"

TEST(BuildFrameTest, BuildEmptyFrame) {
  ACState acs = new_ac_state();
  uint8_t* frame = build_frame(&acs);
   EXPECT_EQ(frame[0], 0xC4);
   EXPECT_EQ(frame[1],0xD3);
   EXPECT_EQ(frame[2],0x64);
   EXPECT_EQ(frame[3],0x80);
   EXPECT_EQ(frame[4],0x00);
   EXPECT_EQ(frame[5],0x06);
   EXPECT_EQ(frame[6],0xC0);
   EXPECT_EQ(frame[7],0xF0);
   EXPECT_EQ(frame[8],0x02);
   EXPECT_EQ(frame[9],0x00);
   EXPECT_EQ(frame[10],0x00);
   EXPECT_EQ(frame[11],0x00);
   EXPECT_EQ(frame[12],0x01);
   EXPECT_EQ(frame[13],0xE2);

  delete[] frame;
}

TEST(BuildFrameTest, Temperature16) {
    ACState acs = new_ac_state();
    set_temperature(&acs, 16);

    uint8_t* frame = build_frame(&acs);

    EXPECT_EQ(frame[7] & 0xF0, 0xF0);

    delete[] frame;
}

TEST(BuildFrameTest, Temperature17) {
    ACState acs = new_ac_state();
    set_temperature(&acs, 17);

    uint8_t* frame = build_frame(&acs);

    EXPECT_EQ(frame[7] & 0xF0, 0x70);

    delete[] frame;
}

TEST(BuildFrameTest, Temperature22) {
    ACState acs = new_ac_state();
    set_temperature(&acs, 22);

    uint8_t* frame = build_frame(&acs);

    EXPECT_EQ(frame[7] & 0xF0, 0x90);

    delete[] frame;
}

TEST(BuildFrameTest, Temperature31) {
    ACState acs = new_ac_state();
    set_temperature(&acs, 31);

    uint8_t* frame = build_frame(&acs);

    EXPECT_EQ(frame[7] & 0xF0, 0x00);

    delete[] frame;
}

TEST(BuildFrameTest, FanAuto) {
    ACState acs = new_ac_state();
    set_fan_mode(&acs, FAN_AUTO);

    uint8_t* frame = build_frame(&acs);

    EXPECT_EQ(frame[8] & 0b11100000, 0b00000000);

    delete[] frame;
}

TEST(BuildFrameTest, FanLow) {
    ACState acs = new_ac_state();
    set_fan_mode(&acs, FAN_LOW);

    uint8_t* frame = build_frame(&acs);

    EXPECT_EQ(frame[8] & 0b11100000, 0b01000000);

    delete[] frame;
}

TEST(BuildFrameTest, FanMiddle) {
    ACState acs = new_ac_state();
    set_fan_mode(&acs, FAN_MIDDLE);

    uint8_t* frame = build_frame(&acs);

    EXPECT_EQ(frame[8] & 0b11100000, 0b11000000);

    delete[] frame;
}

TEST(BuildFrameTest, FanHigh) {
    ACState acs = new_ac_state();
    set_fan_mode(&acs, FAN_HIGH);

    uint8_t* frame = build_frame(&acs);

    EXPECT_EQ(frame[8] & 0b11100000, 0b10100000);

    delete[] frame;
}

TEST(BuildFrameTest, SwingOff) {
    ACState acs = new_ac_state();
    set_swing(&acs, false);

    uint8_t* frame = build_frame(&acs);

    EXPECT_EQ(frame[8] & 0b00011100, 0b00000000);

    delete[] frame;
}

TEST(BuildFrameTest, SwingOn) {
    ACState acs = new_ac_state();
    set_swing(&acs, true);

    uint8_t* frame = build_frame(&acs);

    EXPECT_EQ(frame[8] & 0b00011100, 0b00011100);

    delete[] frame;
}

TEST(BuildFrameTest, SuperOff) {
    ACState acs = new_ac_state();
    set_super(&acs, false);

    uint8_t* frame = build_frame(&acs);

    EXPECT_EQ(frame[6] & (1 << 1), 0);

    delete[] frame;
}

TEST(BuildFrameTest, SuperOn) {
    ACState acs = new_ac_state();
    set_super(&acs, true);

    uint8_t* frame = build_frame(&acs);

    EXPECT_NE(frame[6] & (1 << 1), 0);

    delete[] frame;
}
TEST(BuildFrameTest, SuperForcesTemperature16) {
    ACState acs = new_ac_state();

    set_temperature(&acs, 25);
    set_super(&acs, true);

    uint8_t* frame = build_frame(&acs);

    EXPECT_EQ(frame[7] & 0xF0, 0xF0);

    delete[] frame;
}

TEST(BuildFrameTest, SuperForcesFanAuto) {
    ACState acs = new_ac_state();

    set_fan_mode(&acs, FAN_HIGH);
    set_super(&acs, true);

    uint8_t* frame = build_frame(&acs);

    EXPECT_EQ(frame[8] & 0b11100000, 0b00000000);

    delete[] frame;
}

TEST(BuildFrameTest, PowerOn) {
    ACState acs = new_ac_state();
    set_power(&acs, POWER_TO_ON);

    uint8_t* frame = build_frame(&acs);

    EXPECT_NE(frame[5] & (1 << 5), 0);

    delete[] frame;
}

TEST(BuildFrameTest, PowerOff) {
    ACState acs = new_ac_state();
    set_power(&acs, POWER_TO_OFF);

    uint8_t* frame = build_frame(&acs);

    EXPECT_EQ(frame[5] & (1 << 5), 0);

    delete[] frame;
}
