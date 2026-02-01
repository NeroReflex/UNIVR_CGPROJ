#include "dds_loader.hpp"

std::shared_ptr<DDSResource> load_dds(
    const std::filesystem::path& texture_path,
    DDSLoadResult& load_result
) noexcept {
    const auto file_size = std::filesystem::file_size(texture_path);

    std::cout << "Loading texture (" << file_size << " bytes) from: " << texture_path << std::endl;

    std::ifstream file(texture_path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open DDS file: " << texture_path << std::endl;
        load_result = DDSLoadResult::FileNotFound;
        return nullptr;
    }

    uint32_t magic = 0;
    file.read(reinterpret_cast<char*>(&magic), sizeof(uint32_t));
    if (magic != 0x20534444) { // 'DDS '
        std::cerr << "Invalid DDS file format: " << texture_path << std::endl;
        load_result = DDSLoadResult::InvalidFormat;
        return nullptr;
    }

    auto header = std::make_unique<DDSHeader>();
    file.read(reinterpret_cast<char*>(header.get()), sizeof(DDSHeader));

    // Process DX10 header if needed
    auto header_dx10 = std::unique_ptr<DDSHeaderDXT10>(nullptr);
    if (((header->ddspf.flags & DDS_PIXELFORMAT_FLAG_FOURCC) != 0) &&
        (header->ddspf.fourCC == DDS_FOURCC_DX10)
    ) {
        header_dx10 = std::make_unique<DDSHeaderDXT10>();
        file.read(reinterpret_cast<char*>(header_dx10.get()), sizeof(DDSHeaderDXT10));
    }

    std::vector<uint8_t> data(file_size - file.tellg());
    file.read(reinterpret_cast<char*>(data.data()), data.size());
    if (!file) {
        std::cerr << "Error reading DDS data from file: " << texture_path << std::endl;
        load_result = DDSLoadResult::ReadError;
        return nullptr;
    }

    load_result = DDSLoadResult::Success;
    return std::make_shared<DDSResource>(
        std::move(header),
        std::move(header_dx10),
        std::move(data)
    );
}
