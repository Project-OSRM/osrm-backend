#pragma once

#include "string_view.hpp"
#include <array>
#include <cstring>

namespace sol {
	// Everything here was lifted pretty much straight out of
	// ogonek, because fuck figuring it out=
	namespace unicode {
		enum class error_code {
			ok = 0,
			invalid_code_point,
			invalid_code_unit,
			invalid_leading_surrogate,
			invalid_trailing_surrogate,
			sequence_too_short,
			overlong_sequence,
		};

		inline const string_view& to_string(error_code ec) {
			static const string_view arr[4] = {
				"ok",
				"invalid code points",
				"invalid code unit",
				"overlong sequence"
			};
			return arr[static_cast<std::size_t>(ec)];
		}

		template <typename It>
		struct decoded_result {
			error_code error;
			char32_t codepoint;
			It next;
		};

		template <typename C>
		struct encoded_result {
			error_code error;
			std::size_t code_units_size;
			std::array<C, 4> code_units;
		};

		struct unicode_detail {
			// codepoint related
			static constexpr char32_t last_code_point = 0x10FFFF;

			static constexpr char32_t first_lead_surrogate = 0xD800;
			static constexpr char32_t last_lead_surrogate = 0xDBFF;

			static constexpr char32_t first_trail_surrogate = 0xDC00;
			static constexpr char32_t last_trail_surrogate = 0xDFFF;

			static constexpr char32_t first_surrogate = first_lead_surrogate;
			static constexpr char32_t last_surrogate = last_trail_surrogate;

			static constexpr bool is_lead_surrogate(char32_t u) {
				return u >= first_lead_surrogate && u <= last_lead_surrogate;
			}
			static constexpr bool is_trail_surrogate(char32_t u) {
				return u >= first_trail_surrogate && u <= last_trail_surrogate;
			}
			static constexpr bool is_surrogate(char32_t u) {
				return u >= first_surrogate && u <= last_surrogate;
			}

			// utf8 related
			static constexpr auto last_1byte_value = 0x7Fu;
			static constexpr auto last_2byte_value = 0x7FFu;
			static constexpr auto last_3byte_value = 0xFFFFu;

			static constexpr auto start_2byte_mask = 0x80u;
			static constexpr auto start_3byte_mask = 0xE0u;
			static constexpr auto start_4byte_mask = 0xF0u;

			static constexpr auto continuation_mask = 0xC0u;
			static constexpr auto continuation_signature = 0x80u;

			static constexpr int sequence_length(unsigned char b) {
				return (b & start_2byte_mask) == 0 ? 1
					: (b & start_3byte_mask) != start_3byte_mask ? 2
					: (b & start_4byte_mask) != start_4byte_mask ? 3
					: 4;
			}

			static constexpr char32_t decode(unsigned char b0, unsigned char b1) {
				return ((b0 & 0x1F) << 6) | (b1 & 0x3F);
			}
			static constexpr char32_t decode(unsigned char b0, unsigned char b1, unsigned char b2) {
				return ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
			}
			static constexpr char32_t decode(unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3) {
				return ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
			}

			// utf16 related
			static constexpr char32_t last_bmp_value = 0xFFFF;
			static constexpr char32_t normalizing_value = 0x10000;
			static constexpr int lead_surrogate_bitmask = 0xFFC00;
			static constexpr int trail_surrogate_bitmask = 0x3FF;
			static constexpr int lead_shifted_bits = 10;
			static constexpr char32_t replacement = 0xFFFD;

			static char32_t combine_surrogates(char16_t lead, char16_t trail) {
				auto hi = lead - first_lead_surrogate;
				auto lo = trail - first_trail_surrogate;
				return normalizing_value + ((hi << lead_shifted_bits) | lo);
			}
		};

		inline encoded_result<char> code_point_to_utf8(char32_t codepoint) {
			encoded_result<char> er;
			er.error = error_code::ok;
			if (codepoint <= unicode_detail::last_1byte_value) {
				er.code_units_size = 1;
				er.code_units = std::array<char, 4>{ { static_cast<char>(codepoint) } };
			}
			else if (codepoint <= unicode_detail::last_2byte_value) {
				er.code_units_size = 2;
				er.code_units = std::array<char, 4>{{
					static_cast<char>(0xC0 | ((codepoint & 0x7C0) >> 6)),
					static_cast<char>(0x80 | (codepoint & 0x3F)),
				}};
			}
			else if (codepoint <= unicode_detail::last_3byte_value) {
				er.code_units_size = 3;
				er.code_units = std::array<char, 4>{{
					static_cast<char>(0xE0 | ((codepoint & 0xF000) >> 12)),
					static_cast<char>(0x80 | ((codepoint & 0xFC0) >> 6)),
					static_cast<char>(0x80 | (codepoint & 0x3F)),
				}};
			}
			else {
				er.code_units_size = 4;
				er.code_units = std::array<char, 4>{ {
					static_cast<char>(0xF0 | ((codepoint & 0x1C0000) >> 18)),
						static_cast<char>(0x80 | ((codepoint & 0x3F000) >> 12)),
						static_cast<char>(0x80 | ((codepoint & 0xFC0) >> 6)),
						static_cast<char>(0x80 | (codepoint & 0x3F)),
				} };
			}
			return er;
		}

		inline encoded_result<char16_t> code_point_to_utf16(char32_t codepoint) {
			encoded_result<char16_t> er;

			if (codepoint <= unicode_detail::last_bmp_value) {
				er.code_units_size = 1;
				er.code_units = std::array<char16_t, 4>{ { static_cast<char16_t>(codepoint) } };
				er.error = error_code::ok;
			}
			else {
				auto normal = codepoint - unicode_detail::normalizing_value;
				auto lead = unicode_detail::first_lead_surrogate + ((normal & unicode_detail::lead_surrogate_bitmask) >> unicode_detail::lead_shifted_bits);
				auto trail = unicode_detail::first_trail_surrogate + (normal & unicode_detail::trail_surrogate_bitmask);
				er.code_units = std::array<char16_t, 4>{ {
					static_cast<char16_t>(lead),
					static_cast<char16_t>(trail)
				} };
				er.code_units_size = 2;
				er.error = error_code::ok;
			}
			return er;
		}

		inline encoded_result<char32_t> code_point_to_utf32(char32_t codepoint) {
			encoded_result<char32_t> er;
			er.code_units_size = 1;
			er.code_units[0] = codepoint;
			er.error = error_code::ok;
			return er;
		}

		template <typename It>
		inline decoded_result<It> utf8_to_code_point(It it, It last) {
			decoded_result<It> dr;
			if (it == last) {
				dr.next = it;
				dr.error = error_code::sequence_too_short;
				return dr;
			}

			unsigned char b0 = *it;
			std::size_t length = unicode_detail::sequence_length(b0);

			if (length == 1) {
				dr.codepoint = static_cast<char32_t>(b0);
				dr.error = error_code::ok;
				++it;
				dr.next = it;
				return dr;
			}

			auto is_invalid = [](unsigned char b) { return b == 0xC0 || b == 0xC1 || b > 0xF4; };
			auto is_continuation = [](unsigned char b) {
				return (b & unicode_detail::continuation_mask) == unicode_detail::continuation_signature;
			};

			if (is_invalid(b0) || is_continuation(b0)) {
				dr.error = error_code::invalid_code_unit;
				dr.next = it;
				return dr;
			}

			++it;
			std::array<unsigned char, 4> b;
			b[0] = b0;
			for (std::size_t i = 1; i < length; ++i) {
				b[i] = *it;
				if (!is_continuation(b[i])) {
					dr.error = error_code::invalid_code_unit;
					dr.next = it;
					return dr;
				}
				++it;
			}

			char32_t decoded;
			switch (length) {
			case 2:
				decoded = unicode_detail::decode(b[0], b[1]);
				break;
			case 3:
				decoded = unicode_detail::decode(b[0], b[1], b[2]);
				break;
			default:
				decoded = unicode_detail::decode(b[0], b[1], b[2], b[3]);
				break;
			}

			auto is_overlong = [](char32_t u, std::size_t bytes) {
				return u <= unicode_detail::last_1byte_value
					|| (u <= unicode_detail::last_2byte_value && bytes > 2)
					|| (u <= unicode_detail::last_3byte_value && bytes > 3);
			};
			if (is_overlong(decoded, length)) {
				dr.error = error_code::overlong_sequence;
				return dr;
			}
			if (unicode_detail::is_surrogate(decoded) || decoded > unicode_detail::last_code_point) {
				dr.error = error_code::invalid_code_point;
				return dr;
			}
			
			// then everything is fine
			dr.codepoint = decoded;
			dr.error = error_code::ok;
			dr.next = it;
			return dr;
		}

		template <typename It>
		inline decoded_result<It> utf16_to_code_point(It it, It last) {
			decoded_result<It> dr;
			if (it == last) {
				dr.next = it;
				dr.error = error_code::sequence_too_short;
				return dr;
			}

			char16_t lead = static_cast<char16_t>(*it);
			
			if (!unicode_detail::is_surrogate(lead)) {
				++it;
				dr.codepoint = static_cast<char32_t>(lead);
				dr.next = it;
				dr.error = error_code::ok;
				return dr;
			}
			if (!unicode_detail::is_lead_surrogate(lead)) {
				dr.error = error_code::invalid_leading_surrogate;
				dr.next = it;
				return dr;
			}

			++it;
			auto trail = *it;
			if (!unicode_detail::is_trail_surrogate(trail)) {
				dr.error = error_code::invalid_trailing_surrogate;
				dr.next = it;
				return dr;
			}
			
			dr.codepoint = unicode_detail::combine_surrogates(lead, trail);
			dr.next = ++it;
			dr.error = error_code::ok;
			return dr;
		}

		template <typename It>
		inline decoded_result<It> utf32_to_code_point(It it, It last) {
			decoded_result<It> dr;
			if (it == last) {
				dr.next = it;
				dr.error = error_code::sequence_too_short;
				return dr;
			}
			dr.codepoint = static_cast<char32_t>(*it);
			dr.next = ++it;
			dr.error = error_code::ok;
			return dr;
		}
	}
}
