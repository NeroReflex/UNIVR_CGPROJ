#pragma once

#include "dds_header.hpp"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <functional>
#include <cstdint>

enum class DDSLoadResult {
    Success = 0,
    FileNotFound,
    InvalidFormat,
    UnsupportedFormat,
    ReadError
};

std::shared_ptr<DDSResource> load_dds(
    const std::filesystem::path& texture_path,
    DDSLoadResult& load_result
) noexcept;
