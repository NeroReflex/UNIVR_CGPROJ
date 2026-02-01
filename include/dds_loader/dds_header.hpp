#pragma once

#include <cstdint>
#include <vector>
#include <memory>

#define DDS_PIXELFORMAT_FLAG_FOURCC ((uint32_t)0x00000004)
#define DDS_PIXELFORMAT_FLAG_DDS_RGB ((uint32_t)0x00000040)
#define DDS_PIXELFORMAT_FLAG_DDS_RGBA ((uint32_t)0x00000041)
#define DDS_PIXELFORMAT_FLAG_DDS_LUMINANCE ((uint32_t)0x00020000)
#define DDS_PIXELFORMAT_FLAG_DDS_LUMINANCEA ((uint32_t)0x00020001)
#define DDS_PIXELFORMAT_FLAG_DDS_ALPHA ((uint32_t)0x00000002)
#define DDS_PIXELFORMAT_FLAG_DDS_PAL8 ((uint32_t)0x00000020)

#define DDS_FOURCC_DX10 ((uint32_t)0x30315844)
#define DDS_FOURCC_DXT1 ((uint32_t)0x31347844)
#define DDS_FOURCC_DXT2 ((uint32_t)0x32347844)
#define DDS_FOURCC_DXT3 ((uint32_t)0x33347844)
#define DDS_FOURCC_DXT4 ((uint32_t)0x34347844)
#define DDS_FOURCC_DXT5 ((uint32_t)0x35347844)
#define DDS_FOURCC_ATI1 ((uint32_t)0x31394941)
#define DDS_FOURCC_ATI2 ((uint32_t)0x32394941)
#define DDS_FOURCC_BC4U ((uint32_t)0x55344342)
#define DDS_FOURCC_BC4S ((uint32_t)0x53344342)
#define DDS_FOURCC_BC5U ((uint32_t)0x55354342)
#define DDS_FOURCC_BC5S ((uint32_t)0x53354342)
#define DDS_FOURCC_RGBG ((uint32_t)0x47424752)

#pragma pack(push, 1)

struct DDSPixelFormat {
    uint32_t size;
    uint32_t flags;
    uint32_t fourCC;
    uint32_t RGBBitCount;
    uint32_t RBitMask;
    uint32_t GBitMask;
    uint32_t BBitMask;
    uint32_t ABitMask;
};

struct DDSHeader {
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitchOrLinearSize;
    uint32_t depth;
    uint32_t mipMapCount;
    uint32_t reserved1[11];

    struct DDSPixelFormat ddspf;

    uint32_t caps;
    uint32_t caps2;
    uint32_t caps3;
    uint32_t caps4;
    uint32_t reserved2;
};

struct DDSHeaderDXT10 {
    uint32_t dxgiFormat;
    uint32_t resourceDimension;
    uint32_t miscFlag;
    uint32_t arraySize;
    uint32_t miscFlags2;
};

#pragma pack(pop)

class DDSResource {
public:
    DDSResource(
        std::unique_ptr<DDSHeader>&& h,
        std::unique_ptr<DDSHeaderDXT10>&& h10,
        std::vector<uint8_t>&& data
    ) noexcept;

    bool has_dx10_header() const noexcept;

    uint32_t get_width() const noexcept;

    uint32_t get_height() const noexcept;

    uint32_t get_mipmap_count() const noexcept;

    uint32_t get_size() const noexcept;

    void const* get_data() const noexcept;

protected:
    std::unique_ptr<DDSHeader> header;
    std::unique_ptr<DDSHeaderDXT10> header_dx10;
    std::vector<uint8_t> data;
};

