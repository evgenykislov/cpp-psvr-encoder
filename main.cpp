#include <iostream>
#include <sstream>

#include <stdlib.h>

#include <boost/filesystem.hpp>

extern "C" {
#include <libavformat/avformat.h>
}

namespace fs = boost::filesystem;

enum VideoType {
  kVType_SBS,
  kVType_OU,
  kVType_MONO,
  kVType_2D
};

static fs::path g_input_file_;
static fs::path g_output_path_;
static fs::path g_output_file_;

static std::string g_target_platform_ = "psvr";
static std::string g_degree_str_ = "0";
static unsigned long g_degree_ = 0;
static std::string g_video_type_str_ = "mono";
static VideoType g_video_type_ = kVType_MONO;


void PrintHelp() {
  std::cout << "" << std::endl;
  std::cout << "Required Parameters:" << std::endl;
  std::cout << "  -i input_file - source file for encoding" << std::endl;
  std::cout << "Optional Parameters:" << std::endl;
  std::cout << "  -d degree - video degree. 0 (default) - for flat frame" << std::endl;
  std::cout << "  -o output_folder - folder for output file. Default: forlder of source file" << std::endl;
  std::cout << "  -t video_type - Available types: sbs, ou, mono, 2d. Default: mono" << std::endl;
}

void ParseArguments(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    std::string at = argv[i];
    int ni = i + 1;
    bool niv = ni < argc;
    if (at == "-d" && niv) {
      g_degree_str_ = argv[ni];
      ++i;
      continue;
    }
    if (at == "-i" && niv) {
      g_input_file_ = argv[ni];
      ++i;
      continue;
    }
    if (at == "-o" && niv) {
      g_output_path_ = argv[ni];
      ++i;
      continue;
    }
    if (at == "-t" && niv) {
      g_video_type_str_ = argv[ni];
      ++i;
      continue;
    }
  }
}

bool CheckArguments() {
  // Input file
  if (g_input_file_.empty()) {
    std::cerr << "Input file isn't specified" << std::endl;
    return false;
  }
  if (!fs::is_regular_file(g_input_file_)) {
    // File isn't exist
    std::cerr << "Input file isn't exist" << std::endl;
    return false;
  }

  // Degree
  const char* dgr_begin = g_degree_str_.c_str();
  const char* dgr_end = dgr_begin + g_degree_str_.size();
  char* real_end = nullptr;
  g_degree_ = strtoul(dgr_begin, &real_end, 10);
  if (real_end != dgr_end) {
    std::cerr << "Wrong video degree argument" << std::endl;
    return false;
  }
  if (!(g_degree_ == 0 || g_degree_ == 180 || g_degree_ == 360)) {
    std::cerr << "Wrong video degree value" << std::endl;
    return false;
  }

  // Video Type
  if (g_video_type_str_ == "sbs") {
    g_video_type_ = kVType_SBS;
  }
  else if (g_video_type_str_ == "ou") {
    g_video_type_ = kVType_OU;
  }
  else if (g_video_type_str_ == "mono") {
    g_video_type_ = kVType_MONO;
  }
  else if (g_video_type_str_ == "2d") {
    g_video_type_ = kVType_2D;
  }
  else {
    std::cerr << "Wrong type of video" << std::endl;
    return false;
  }

  // Output path/file
  if (g_output_path_.empty()) {
    g_output_path_ = g_input_file_.parent_path();
  }
  std::stringstream name;
  name << g_input_file_.stem().string() << "_" << g_target_platform_;
  switch (g_degree_) {
    case 0:
      if (g_video_type_ == kVType_MONO || g_video_type_ == kVType_2D) {
        name << "_2dff.mp4";
      }
      else if (g_video_type_ == kVType_SBS || g_video_type_ == kVType_OU) {
        name << "_3dff_" << g_video_type_str_ << ".mp4";
      }
      else {
        assert(false);
        std::cerr << "Logical error" << std::endl;
        return false;
      }
      break;
    case 180:
    case 360:
      name << g_degree_str_ + "_" << g_video_type_str_ << ".mp4";
      break;
  }
  g_output_file_ = g_output_path_ / name.str();

  return true;
}

void GetVideoInfo() {
  auto ctx = avformat_alloc_context();
  if (!ctx) {
    return;
  }
  if (avformat_open_input(&ctx, g_input_file_.c_str(), nullptr, nullptr) == 0) {
    if (avformat_find_stream_info(ctx, nullptr) >= 0) {
      std::cout << "Duration: " << ctx->duration << std::endl;
      std::cout << "Bitrate: " << ctx->bit_rate << std::endl;

      for (unsigned int i = 0; i < ctx->nb_streams; ++i) {
        auto& cdc = ctx->streams[i]->codecpar;
        if (cdc->codec_type != AVMEDIA_TYPE_VIDEO) {
          continue;
        }

        std::cout << "Width: " << cdc->width << std::endl;
        std::cout << "Height: " << cdc->height << std::endl;
        break;
      }

    }
    avformat_close_input(&ctx);
  }
  avformat_free_context(ctx);
}

int main(int argc, char** argv)
{
  if (argc <= 1) {
    PrintHelp();
    return 1;
  }

  ParseArguments(argc, argv);
  if (!CheckArguments()) { return 1; }
  // Detect video parameters
  GetVideoInfo();


  return 0;
}
