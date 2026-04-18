
#include "convert_endian.hpp"

#include <gtest/gtest.h>

// READ FROM BIG ENDIAN

TEST(ConvertEndian, should_return_correct_uint16_when_bytes_are_valid) {
  // Arrange
  std::vector<std::uint8_t> buffer = {0b00000001,
                                      0b00101100};  // decimal value = 300
  std::uint16_t expected = 300;

  // Act
  std::uint16_t result = ConvertEndian::readU16BE(buffer, 0);

  // Assert
  EXPECT_EQ(expected, result);
}

TEST(ConvertEndian, should_return_correct_uint16_when_value_is_zero) {
  // Arrange
  std::vector<std::uint8_t> buffer = {0b00000000,
                                      0b00000000};  // decimal value = 0
  std::uint16_t expected = 0;

  // Act
  std::uint16_t result = ConvertEndian::readU16BE(buffer, 0);

  // Assert
  EXPECT_EQ(expected, result);
}

TEST(ConvertEndian, should_return_correct_uint16_when_value_is_65535) {
  // Arrange
  std::vector<std::uint8_t> buffer = {0b11111111,
                                      0b11111111};  // decimal value = 65535
  std::uint16_t expected = 65535;

  // Act
  std::uint16_t result = ConvertEndian::readU16BE(buffer, 0);

  // Assert
  EXPECT_EQ(expected, result);
}

TEST(ConvertEndian, should_read_from_correct_position_when_offset_is_provided) {
  // Arrange
  std::vector<std::uint8_t> buffer = {0x00, 0x00, 0b00000001, 0b00101100};
  std::size_t offset = 2;
  std::uint16_t expected = 300;

  // Act
  std::uint16_t result = ConvertEndian::readU16BE(buffer, offset);

  // Assert
  EXPECT_EQ(expected, result);
}

// WRITE TO BIG ENDIAN
TEST(ConvertEndian, should_return_correct_bytes_when_unint16_is_valid) {
  // Arrange
  std::uint16_t toWrite = 300;
  std::vector<std::uint8_t> expected = {0b00000001,
                                        0b00101100};  // decimal value = 300
  std::size_t offset = 0;

  std::vector<std::uint8_t> result;
  result.resize(2);

  // Act
  ConvertEndian::writeU16BE(result, offset, toWrite);

  // Assert
  EXPECT_EQ(expected[offset], result[offset]);
  EXPECT_EQ(expected[offset + 1], result[offset + 1]);
}

TEST(ConvertEndian, should_return_correct_bytes_when_unint16_is_0) {
  // Arrange
  std::uint16_t toWrite = 0;
  std::vector<std::uint8_t> expected = {0b00000000,
                                        0b00000000};  // decimal value = 0
  std::size_t offset = 0;

  std::vector<std::uint8_t> result;
  result.resize(2);

  // Act
  ConvertEndian::writeU16BE(result, offset, toWrite);

  // Assert
  EXPECT_EQ(expected[offset], result[offset]);
  EXPECT_EQ(expected[offset + 1], result[offset + 1]);
}

TEST(ConvertEndian, should_return_correct_bytes_when_unint16_is_65535) {
  // Arrange
  std::uint16_t toWrite = 65535;
  std::vector<std::uint8_t> expected = {0b11111111,
                                        0b11111111};  // decimal value = 65535
  std::size_t offset = 0;

  std::vector<std::uint8_t> result;
  result.resize(2);

  // Act
  ConvertEndian::writeU16BE(result, offset, toWrite);

  // Assert
  EXPECT_EQ(expected[offset], result[offset]);
  EXPECT_EQ(expected[offset + 1], result[offset + 1]);
}

// WRITE TO BIG ENDIAN
TEST(ConvertEndian, should_return_correct_bytes_when_offset_is_provided) {
  // Arrange
  std::uint16_t toWrite = 300;
  std::vector<std::uint8_t> expected = {0b00000000, 0b00000000, 0b00000001,
                                        0b00101100};  // decimal value = 300
  std::size_t offset = 2;

  std::vector<std::uint8_t> result;
  result.resize(4);

  // Act
  ConvertEndian::writeU16BE(result, offset, toWrite);

  // Assert
  EXPECT_EQ(expected[offset], result[offset]);
  EXPECT_EQ(expected[offset + 1], result[offset + 1]);
}

TEST(ConvertEndian,
     should_return_original_value_when_read_is_called_after_write) {
  // Arrange
  std::uint16_t original = 539;
  std::size_t offset = 1;
  std::vector<std::uint8_t> buffer;
  buffer.resize(3);

  // Act
  ConvertEndian::writeU16BE(buffer, offset, original);
  std::uint16_t result = ConvertEndian::readU16BE(buffer, offset);

  // Assert
  EXPECT_EQ(original, result);
}