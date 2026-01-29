/*!
 * @file XCDFRoot.cc 
 * @brief Generically convert XCDF files to ROOT with incremental writing.
 * @author Jim Braun 
 * @date 17 Feb 2012 
 * @version $Id$
 * @brief modified by Wang, zhen, Detach aerie-io/XCDFRoot.cc from aerie; You must install XCDF and root already; compile g++ -o xcdf2root xcdf2root.cpp `root-config --cflags --libs` -I/path/xcdf/install/include/ -L/path/xcdf/install/lib  -lxcdf -Wl,-rpath,/path/xcdf/install/lib 
 * @date  Feb fifth 2024
 */

#include <xcdf/XCDF.h>

#include <iostream>
#include <vector>
#include <string>

#include <TFile.h>
#include <TTree.h>
#include <TNamed.h>




#define MAX_TNAMED_LENGTH 1024

typedef std::vector<std::string> FileList;

void printHelp() {
    std::cout << "Usage: program [options]\n"
              << "Options:\n"
              << "  -i <file1> <file2> ...   Input file names\n"
              << "  -o <filename>            Output file name\n"
              << "  --comment                 keep comments\n"
              << "  -h, --help                Show this help message\n";
}

void GetDescription(const std::string& name,
                    const std::string& parentName,
                    std::string& out,
                    const char type) {
    if (!parentName.empty()) {
        out = name + "[" + parentName + "]";
    } else {
        out = name;
    }

    out += (type == XCDF_UNSIGNED_INTEGER) ? "/l" : (type == XCDF_SIGNED_INTEGER) ? "/L" : "/D";
//     if (type == XCDF_UNSIGNED_INTEGER) {
//     out += "/l";
//   } else if (type == XCDF_SIGNED_INTEGER) {
//     out += "/L";
//   } else {
//     out += "/D";
//   }
}

class SetBranchAddressVisitor {
public:
    SetBranchAddressVisitor(TTree* tree) : tree_(tree) { }

    template <typename T>
    void operator()(const XCDFField<T>& field) {
        void* address = const_cast<T*>(&(field[0]));
        tree_->SetBranchAddress(field.GetName().c_str(), address);
    }

private:
    TTree* tree_;
};

// void show_duration(std::string pinpoint) {
//     time_point time2 = high_resolution_clock::now();
//     milliseconds  duration = duration_cast<milliseconds>(time2 - time1); // or milliseconds, nanoseconds
//     std::cout <<pinpoint<< "Execution time: " << duration.count() << " ms\n";
//     time1 = time2;
// }


int main(int argc, char** argv) {

    FileList infileNames;
    TFile* outFile = nullptr;
    TTree* out = nullptr;
    uint64_t entryCount = 0;
    XCDFFile xcdfile;

    std::string outputFile;
    bool Comment = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printHelp();
            return 0;
        } else if (arg == "-i") {
            while (++i < argc && argv[i][0] != '-') {
                infileNames.push_back(argv[i]);
            }
            --i; // Adjust index since loop increments once more
        } else if (arg == "-o") {
            if (++i < argc) outputFile= argv[i];
        } else if (std::string(argv[i]) == "--comment") {
            Comment = true;  // Set the no comment flag
            std::cout<<"Keep Comments"<<std::endl;
        }
    }

    if(outputFile.empty()){
        std::cerr << "Error: No output file specified " << std::endl;
        exit(1);
    }
    if(infileNames.empty()){
        std::cerr << "Error: No input file specified " << std::endl;
        exit(1);
    }


    for (unsigned f = 0; f < infileNames.size(); f++) {
        std::string theFileName = infileNames[f];
        std::cout << "Reading " << f + 1 << "/" << infileNames.size() << ": " << theFileName << std::endl;

        if (!xcdfile.Open(theFileName, "r")) {
            std::cerr << "Cannot open " <<  theFileName << std::endl;
            return 1;
        }


        if (!outFile) {
            outFile = TFile::Open(outputFile.c_str(), "RECREATE", "", 1);
            out = new TTree("XCDF", "XCDF");
            out->SetAutoSave();


            // Create branches using the correct iterator
            for (std::vector<XCDFFieldDescriptor>::const_iterator   it = xcdfile.FieldDescriptorsBegin();  it != xcdfile.FieldDescriptorsEnd(); ++it) {
                std::string description;
                GetDescription(it->name_, it->parentName_, description, it->type_);
                out->Branch(it->name_.c_str(), static_cast<char*>(nullptr), description.c_str());
                std::cout << "Creating Branch: " << it->name_.c_str() << " \"" << description.c_str() << "\"" << std::endl;
            }
            if(Comment){
                for (std::vector<std::string>::const_iterator  it = xcdfile.CommentsBegin(); it != xcdfile.CommentsEnd(); ++it) {
                    if (strlen(it->c_str()) > MAX_TNAMED_LENGTH) {
                        std::cout << "Skipping long comment (" << strlen(it->c_str())
                                        << " bytes)." << std::endl;
                        continue;
                    }
                    TNamed name(it->c_str(), "");
                    name.Write();
                }
            }else{
                std::cout<<"Abandon Comments"<<std::endl;
            }

        }


        SetBranchAddressVisitor setBranchAddressVisitor(out);
        // Read and process entries
        while (xcdfile.Read()) {
            xcdfile.ApplyFieldVisitor(setBranchAddressVisitor);
            out->Fill();
            entryCount++;
        }
        xcdfile.Close();
    }

    // show_duration("pend");
    out->Write();  // Final write to ensure all data is saved
    outFile->Close();
    std::cout << "Wrote ROOT file " << outputFile << ": " << entryCount << " entries" << std::endl;

    return 0;
}
