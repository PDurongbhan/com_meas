/*----------------------------------------------------------------------------\
|  files: COM_MEAS                                                            
||                                                                             
||  description:                                                               
||   Finds the centre of mass of two solid bone masks, calculates the distance 
||   between the two centres of mass and the angles between the connecting     
||   line and all three axes						                              
||									                                          
||  input:  two solid bone masks		            		                      
||  output: text file containing distance between centre of masses and angles  
||          between the connecting line and all three axes 	        	      
||                                                                             
||  author:                                                                    
||                                                                             
||  date: 10.08.2008                                                           
||                                                                             
||  modified:  06.10.2014, Patrick Weber, on VMS at ETH using Aimpack
||             27.06.2019, Pholpat Durongbhan, on VMS at UoM using Aimpack
||             19.01.2026, Pholpat Durongbhan, on Windows at UoM using AimIO
||             21.01.2026, Pholpat Durongbhan, added batch/config mode + robust CSV output						
|\----------------------------------------------------------------------------*/

#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include "AimIO/AimIO.h"
//#include "COM_MEAS.hxx"

namespace fs = std::filesystem;

static const char* USAGE =
R"(USAGE:
  COM_MEAS <file1> <file2> [-o out.csv]
  COM_MEAS <config.txt> [-o out.csv]

CONFIG FORMAT:
  First non-empty, non-comment line: input folder path
  Subsequent non-empty, non-comment lines: "<image1> <image2>"

NOTES:
  - Lines starting with # or // are treated as comments
  - Output defaults to COMMS_RESULT.csv if -o not provided
)";

struct Metrics {
  double dist_mm = 0.0;
  double anglex_deg = 0.0;
  double angley_deg = 0.0;
  double anglez_deg = 0.0;
};

struct Pair {
  std::string f1;
  std::string f2;
};

static inline std::string trim(const std::string& s) {
  const char* ws = " \t\r\n";
  const auto b = s.find_first_not_of(ws);
  if (b == std::string::npos) return "";
  const auto e = s.find_last_not_of(ws);
  return s.substr(b, e - b + 1);
}

static inline bool is_comment_or_empty(const std::string& line) {
  const std::string t = trim(line);
  if (t.empty()) return true;
  if (t.rfind("#", 0) == 0) return true;
  if (t.rfind("//", 0) == 0) return true;
  return false;
}

static inline double clamp01(double x) {
  if (x < -1.0) return -1.0;
  if (x >  1.0) return  1.0;
  return x;
}

static bool parse_config(const fs::path& cfg_path, fs::path& base_dir, std::vector<Pair>& pairs, std::string& err) {
  std::ifstream in(cfg_path);
  if (!in) {
    err = "Could not open config file: " + cfg_path.string();
    return false;
  }

  std::string line;
  bool got_base = false;

  while (std::getline(in, line)) {
    if (is_comment_or_empty(line)) continue;
    base_dir = fs::path(trim(line));
    got_base = true;
    break;
  }

  if (!got_base) {
    err = "Config file has no base directory line (first non-empty, non-comment line).";
    return false;
  }

  while (std::getline(in, line)) {
    if (is_comment_or_empty(line)) continue;

    std::istringstream iss(line);
    Pair p;
    if (!(iss >> p.f1 >> p.f2)) {
      // malformed line; skip but keep going
      std::cerr << "Warning: skipping malformed line in config: " << line << "\n";
      continue;
    }
    pairs.push_back(p);
  }

  if (pairs.empty()) {
    err = "No valid pairs found in config file.";
    return false;
  }

  return true;
}

static bool compute_com_and_metrics(const fs::path& file1, const fs::path& file2, Metrics& out, std::string& err) {
  // Create AimFile objects
  AimIO::AimFile in1Aim;
  AimIO::AimFile in2Aim;

  in1Aim.filename = file1.string().c_str();
  in2Aim.filename = file2.string().c_str();

  // Read headers
  try {
    in1Aim.ReadImageInfo();
  } catch (...) {
    err = "Failed to read header for file1: " + file1.string();
    return false;
  }
  try {
    in2Aim.ReadImageInfo();
  } catch (...) {
    err = "Failed to read header for file2: " + file2.string();
    return false;
  }

  if (in1Aim.buffer_type != AimIO::AimFile::AIMFILE_TYPE_CHAR) {
    err = "file1 buffer_type is not AIMFILE_TYPE_CHAR: " + file1.string();
    return false;
  }
  if (in2Aim.buffer_type != AimIO::AimFile::AIMFILE_TYPE_CHAR) {
    err = "file2 buffer_type is not AIMFILE_TYPE_CHAR: " + file2.string();
    return false;
  }

  const int64_t dimx1 = in1Aim.dimensions[0];
  const int64_t dimy1 = in1Aim.dimensions[1];
  const int64_t dimz1 = in1Aim.dimensions[2];

  const int64_t dimx2 = in2Aim.dimensions[0];
  const int64_t dimy2 = in2Aim.dimensions[1];
  const int64_t dimz2 = in2Aim.dimensions[2];

  const int64_t offsetx1 = in1Aim.position[0];
  const int64_t offsety1 = in1Aim.position[1];
  const int64_t offsetz1 = in1Aim.position[2];

  const int64_t offsetx2 = in2Aim.position[0];
  const int64_t offsety2 = in2Aim.position[1];
  const int64_t offsetz2 = in2Aim.position[2];

  const double elsize1 = static_cast<double>(in1Aim.element_size[0]);
  const double elsize2 = static_cast<double>(in2Aim.element_size[0]);

  // Warn if voxel sizes differ
  if (std::abs(elsize1 - elsize2) > 1e-9) {
    std::cerr << "Warning: element_size differs between images ("
              << elsize1 << " vs " << elsize2 << "). Using file1 element_size.\n";
  }

  // Read image data
  size_t size1 = long_product(in1Aim.dimensions);
  std::vector<char> image_data1(size1);
  try {
    in1Aim.ReadImageData(image_data1.data(), size1);
  } catch (...) {
    err = "Failed to read image data for file1: " + file1.string();
    return false;
  }

  size_t size2 = long_product(in2Aim.dimensions);
  std::vector<char> image_data2(size2);
  try {
    in2Aim.ReadImageData(image_data2.data(), size2);
  } catch (...) {
    err = "Failed to read image data for file2: " + file2.string();
    return false;
  }

  // Compute COMs (in voxel coordinates, including offsets)
  auto compute_com = [](const std::vector<char>& img, int64_t dimx, int64_t dimy, int64_t dimz,
                        int64_t offx, int64_t offy, int64_t offz,
                        double& cx, double& cy, double& cz, int64_t& count) -> bool {
    int64_t sumx = 0, sumy = 0, sumz = 0;
    count = 0;

    for (int64_t k = 0; k < dimz; ++k) {
      for (int64_t j = 0; j < dimy; ++j) {
        const int64_t base = k * dimx * dimy + j * dimx;
        for (int64_t i = 0; i < dimx; ++i) {
          // Treat char as unsigned for safety
          unsigned char v = static_cast<unsigned char>(img[base + i]);
          if (v == 127) {
            sumx += i;
            sumy += j;
            sumz += k;
            ++count;
          }
        }
      }
    }

    if (count == 0) return false;

    cx = (static_cast<double>(sumx) / static_cast<double>(count)) + static_cast<double>(offx);
    cy = (static_cast<double>(sumy) / static_cast<double>(count)) + static_cast<double>(offy);
    cz = (static_cast<double>(sumz) / static_cast<double>(count)) + static_cast<double>(offz);
    return true;
  };

  double cx1=0, cy1=0, cz1=0;
  double cx2=0, cy2=0, cz2=0;
  int64_t count1=0, count2=0;

  if (!compute_com(image_data1, dimx1, dimy1, dimz1, offsetx1, offsety1, offsetz1, cx1, cy1, cz1, count1)) {
    err = "No object voxels (value 127) found in file1: " + file1.string();
    return false;
  }
  if (!compute_com(image_data2, dimx2, dimy2, dimz2, offsetx2, offsety2, offsetz2, cx2, cy2, cz2, count2)) {
    err = "No object voxels (value 127) found in file2: " + file2.string();
    return false;
  }

  const double dx = (cx2 - cx1);
  const double dy = (cy2 - cy1);
  const double dz = (cz2 - cz1);

  const double dist_vox = std::sqrt(dx*dx + dy*dy + dz*dz);
  if (dist_vox <= 0.0) {
    err = "Distance is zero (COMs identical). Cannot compute angles reliably.";
    return false;
  }

  out.dist_mm   = dist_vox * elsize1;
  out.anglex_deg = std::acos(clamp01(dx / dist_vox)) * 57.3;
  out.angley_deg = std::acos(clamp01(dy / dist_vox)) * 57.3;
  out.anglez_deg = std::acos(clamp01(dz / dist_vox)) * 57.3;

  return true;
}

static int run_single(const fs::path& f1, const fs::path& f2, const fs::path& out_csv) {
  std::ofstream out(out_csv);
  if (!out) {
    std::cerr << "Error: could not open output CSV: " << out_csv << "\n";
    return 1;
  }

  out << "File1,File2,Distance between COMS,Angle x,Angle y,Angle z,Status,Error\n";

  Metrics m;
  std::string err;
  bool ok = compute_com_and_metrics(f1, f2, m, err);

  out << f1.filename().string() << ","
      << f2.filename().string() << ",";

  if (ok) {
    out << m.dist_mm << "," << m.anglex_deg << "," << m.angley_deg << "," << m.anglez_deg
        << ",OK,\n";
    std::cout << "Distance (mm): " << m.dist_mm << "\n"
              << "Angles (deg) x=" << m.anglex_deg << " y=" << m.angley_deg << " z=" << m.anglez_deg << "\n";
    return 0;
  } else {
    out << ",,,," << "FAIL," << "\"" << err << "\"\n";
    std::cerr << "FAIL: " << err << "\n";
    return 2;
  }
}

static int run_config(const fs::path& cfg, const fs::path& out_csv) {
  fs::path base_dir;
  std::vector<Pair> pairs;
  std::string perr;

  if (!parse_config(cfg, base_dir, pairs, perr)) {
    std::cerr << "Error parsing config: " << perr << "\n";
    return 1;
  }

  std::ofstream out(out_csv);
  if (!out) {
    std::cerr << "Error: could not open output CSV: " << out_csv << "\n";
    return 1;
  }

  out << "File1,File2,Distance between COMS,Angle x,Angle y,Angle z,Status,Error\n";

  int failures = 0;

  std::cout << "Base directory: " << base_dir.string() << "\n";
  std::cout << "Pairs: " << pairs.size() << "\n";

  for (size_t idx = 0; idx < pairs.size(); ++idx) {
    const fs::path f1 = base_dir / pairs[idx].f1;
    const fs::path f2 = base_dir / pairs[idx].f2;

    Metrics m;
    std::string err;
    bool ok = compute_com_and_metrics(f1, f2, m, err);

    out << fs::path(pairs[idx].f1).filename().string() << ","
        << fs::path(pairs[idx].f2).filename().string() << ",";

    if (ok) {
      out << m.dist_mm << "," << m.anglex_deg << "," << m.angley_deg << "," << m.anglez_deg
          << ",OK,\n";
    } else {
      ++failures;
      out << ",,,," << "FAIL," << "\"" << err << "\"\n";
      std::cerr << "FAIL [" << (idx + 1) << "/" << pairs.size() << "]: " << err << "\n";
    }
  }

  std::cout << "Done. Failures: " << failures << "\n";
  return (failures == 0) ? 0 : 2;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cout << USAGE;
    return 0;
  }

  fs::path out_csv = "COMMS_RESULT.csv";
  std::vector<std::string> positional;

  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "-h" || a == "--help") {
      std::cout << USAGE;
      return 0;
    } else if (a == "-o" || a == "--out") {
      if (i + 1 >= argc) {
        std::cerr << "Error: missing value after " << a << "\n";
        return 1;
      }
      out_csv = fs::path(argv[++i]);
    } else {
      positional.push_back(a);
    }
  }

  if (positional.size() == 2) {
    return run_single(fs::path(positional[0]), fs::path(positional[1]), out_csv);
  } else if (positional.size() == 1) {
    return run_config(fs::path(positional[0]), out_csv);
  } else {
    std::cout << USAGE;
    return 1;
  }
}
