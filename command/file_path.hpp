#pragma once

#include "flag_spec.hpp"

#include "bee/file_path.hpp"

namespace command {
namespace flags {

constexpr auto file_path = create_flag_spec<bee::FilePath>();

}
} // namespace command
