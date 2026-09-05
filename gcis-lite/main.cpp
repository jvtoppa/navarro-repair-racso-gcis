#include <iostream>
#include <fstream>
#include <chrono>
#include <variant>
#include <charconv>
#include "simple8b.h"
#include "headers/eliasfano.h"
#include "headers/gcis.h"
#include "headers/utils.h"
#include "malloc_count.h"
using timer = std::chrono::high_resolution_clock;
using EncoderVariant = std::variant<S8B<32>, EliasFano<32>>;
using sais_index_type = int;

EncoderVariant make_encoder(const std::string& type) {
    if (type == "-s8b") return S8B<32>{};
    return EliasFano<32>{};
}

int main(int argc, char* argv[]) {
 
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <mode: -c|-d> <input_path> <output_path> <encoder: -s8b|-ef> <max level>\n";
        return -1;
    }

    std::string mode(argv[1]);
    std::string input_arg(argv[2]);
    std::string output_arg(argv[3]);
    std::string encoder_type(argv[4]);

    EncoderVariant encoder;
    if (encoder_type == "-s8b") {
        encoder.emplace<S8B<32>>();
    } else if (encoder_type == "-ef") {
        encoder.emplace<EliasFano<32>>();
    } else {
        std::cerr << "Invalid encoder type: " << encoder_type << std::endl;
        return -1;
    }

    if (mode == "-d") {
        std::cerr << "Decompression." << std::endl;

        // Load rules and reduced string using input_arg + extensions
        std::ifstream rules_input(input_arg + ".r", std::ios::binary);
        std::ifstream string_input(input_arg + ".c", std::ios::binary);
        std::ofstream output(output_arg, std::ios::binary);

        if (!rules_input.is_open() || !string_input.is_open() || !output.is_open()) {
            std::cerr << "Error opening input/output files for decompression.\n";
            return -1;
        }

        EncoderVariant encoder_variant = make_encoder(encoder_type);

        std::visit([&]<typename Encoder>(Encoder& enc) {
            GrammarInterface<sais_index_type, Encoder> gi;

            gi.load_rules(rules_input);
            gi.load_reduced_string(string_input);

            auto start = timer::now();
            gi.decode(output);
            auto stop = timer::now();

            std::cout << "GCIS Decompression time: "
                      << static_cast<double>(
                             std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()
                         )
                      << " ms\n";
        }, encoder_variant);

        rules_input.close();
        string_input.close();
        output.close();
    }
    else if (mode == "-c") {
        bool flag = false;
        std::string_view l(argv[5]);
        int max_level = 0;
        std::from_chars(l.data(), l.data() + l.size(), max_level);

        
        std::cerr << "GCIS Compression." << std::endl;
        int n = 0;
        char *str = nullptr;
        load_string_from_file(str, input_arg.c_str(), n);

        std::ofstream rules_output(output_arg + ".r", std::ios::binary);
        std::ofstream string_output(output_arg + ".c", std::ios::binary);

        if (!rules_output.is_open() || !string_output.is_open()) {
            std::cerr << "Error opening output files for compression.\n";
            delete[] str;
            return -1;
        }

        auto SA = new int[n];
        EncoderVariant encoder_variant = make_encoder(encoder_type);
        std::visit([&]<typename Encoder>(Encoder& enc) {
            GrammarInterface<sais_index_type, Encoder> gi;
            malloc_count_reset_peak();
            auto start = timer::now();

            if (gcis(gi, str, SA, n, -1, max_level) == 0)
            {
                auto stop = timer::now();
                size_t peak_compression_memory = malloc_count_peak();
                std::cout << "Max Level = " << max_level << "\n";
                std::cout << "Input:\t" << n << " bytes\n";
                std::cout << "Output (.c):\t" << gi.size_in_bytes() << " bytes\n";
                std::cout << "GCIS Compression Time: "<< static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()) << "ms\n";
                std::cout << "GCIS Compression Peak Memory: " << peak_compression_memory << " bytes\n";
                std::cout << "Compression done successfully\n";
            }
            else
            {
                std::cout << "End." << "\n";
                flag = true;
            }


           

            gi.serialize_rules(rules_output);
            gi.serialize_reduced_string(string_output);

        }, encoder_variant);

        delete[] SA;
        delete[] str;
        rules_output.close();
        string_output.close();
        if(flag) return 0;
        /*
        std::string command = "./repair/irepair " + output_arg + ".c";
        int exit_code = std::system(command.c_str());
        if(exit_code)
        {
            std::cout << "Error executing iRepair, exit code: " << exit_code << "\n";
        }
        */

    }
    else {
        std::cerr << "Incorrect algorithm mode: " << mode << std::endl;
    }

    return 0;
}