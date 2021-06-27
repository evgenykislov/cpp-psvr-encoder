#include <iostream>


#include <boost/filesystem.hpp>

extern "C" {
#include <libavformat/avformat.h>
}

namespace fs = boost::filesystem;

static fs::path g_input_file_;
static fs::path g_output_path_;
static fs::path g_output_file_;

static std::string g_target_platform_ = "psvr";
static unsigned long g_degree_ = 0;

void PrintHelp() {
  std::cout << "" << std::endl;
}

void ParseArguments(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    std::string at = argv[i];
    int ni = i + 1;
    bool niv = ni < argc;
    if (at == "-d" && niv) {
      g_degree_ = strtoul(argv[ni], nullptr, 10);
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
      ++i;
      continue;
    }
  }
}

void FormParameters() {
  if (g_output_path_.empty()) {
    g_output_path_ = g_input_file_.parent_path();
  }
  auto name = g_input_file_.stem().string();
  name += "_" + g_target_platform_;
  g_output_file_ = g_output_path_ / name;
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
  ParseArguments(argc, argv);
  // Check parameters
  if (!fs::is_regular_file(g_input_file_)) {
    // File isn't exist
    std::cerr << "Input file isn't exist" << std::endl;
    return 1;
  }

  FormParameters();


  // Detect video parameters
  GetVideoInfo();




  return 0;
}
