#include "dds_header.hpp"

DDSResource::DDSResource(
    std::unique_ptr<DDSHeader>&& h,
    std::unique_ptr<DDSHeaderDXT10>&& h10,
    std::vector<uint8_t>&& data
) noexcept
    : header(std::move(h)), header_dx10(std::move(h10)), data(std::move(data))
{}

bool DDSResource::has_dx10_header() const noexcept {
    return header && header->ddspf.fourCC == DDS_FOURCC_DX10;
}

uint32_t DDSResource::get_width() const noexcept {
    return header ? header->width : 0;
}

uint32_t DDSResource::get_height() const noexcept {
    return header ? header->height : 0;
}

uint32_t DDSResource::get_mipmap_count() const noexcept {
    if (!header) return 1;
    return header->mipMapCount == 0 ? 1 : header->mipMapCount;
}

uint32_t DDSResource::get_size() const noexcept {
    return data.size();
}

void const* DDSResource::get_data() const noexcept {
    return data.data();
}
