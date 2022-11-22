#include "clang/Driver/Options.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/FrontendOptions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Rewrite/Core/Rewriter.h"

// CPP header files and libs

#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iomanip>
#include <vector>
#include <string>
#include <sstream>
#include <map>

#include <sys/metadata_payloads.h>

// Spaces

using namespace std;
using namespace clang;
using namespace llvm;

// Structs

struct payload_lines{
    string line;
    size_t size_payload;
};

struct payloadhdr_lines{
  string string_function_num;
};

// Global variables

Rewriter rewriter;
vector<string> global_args;

// Limits
#define MAX_ARGUMENTS 4     // Max amount of arguments to be passed to the plugin

// Function declarations

void payload_Binary_fill(int , uint8_t* , string & , long );
void addPayload_line(string , struct payload_lines* );
void create_addPayloadHdr_line(string , struct payloadhdr_lines* );
void fill_payloads_line(string & , int , struct payload_lines* );
void fill_payloadHeaders_line(string & , int , struct payloadhdr_lines* );
void fill_payloadMetadataHdr_line(string & , string );
void merge_lines(string & , string , string , string );
string getMetadataLines(vector<string>);


//////////////////////////////////////////////////////////////////


class RenameVisitor : public RecursiveASTVisitor<RenameVisitor> {
private:
    ASTContext *astContext; // used for getting additional AST info
 
public:
    explicit RenameVisitor(CompilerInstance *CI)
        : astContext(&(CI->getASTContext())) // initialize private members
    {
        rewriter.setSourceMgr(astContext->getSourceManager(),
            astContext->getLangOpts());
    }
 


    virtual bool VisitVarDecl(VarDecl *vardecl) {

        string VarDeclName = vardecl->getName().str();
        if (VarDeclName == "__PAYLOAD__") {
            // 0) Check if no arguments were passed
            if(global_args.empty() == true){
                cout << "No arguments were passed. __PAYLOAD__ is not replaced";
                return false;   // Return false to abort the tree traversing 
            }

            // 1) Obtain the whole line to replace
            string complete_metadata_replace = getMetadataLines(global_args);
            //cout << "\n complete_metadata_replace: " << complete_metadata_replace << "\n";

            // 2) Get the location in source code of __PAYLOAD__
            SourceLocation location = vardecl->getLocation();
            
            // 3) Replace __PAYLOAD__ with the full processed metadata lines
            rewriter.ReplaceText(location.getLocWithOffset(0) , VarDeclName.length(), complete_metadata_replace);
            
            return false;       // Return false to abort the entire traversal of the tree
        }

        return true;
        
    }    

    
};

// 3) 
class RenameASTConsumer : public ASTConsumer {
private:
    RenameVisitor *visitor; // doesn't have to be private

    // Function to get the base name of the file provided by path
    string basename(std::string path) {
        return std::string( std::find_if(path.rbegin(), path.rend(), MatchPathSeparator()).base(), path.end());
    }

    // Used by std::find_if
    struct MatchPathSeparator
    {
        bool operator()(char ch) const {
            return ch == '/';
        }
    };
 
public:
    explicit RenameASTConsumer(CompilerInstance *CI)
        // Initialize the visitor. Calls RenameVisitor
        : visitor(new RenameVisitor(CI)) 
        { }
 
    virtual void HandleTranslationUnit(ASTContext &Context) {
        // Traverse across the AST tree
        visitor->TraverseDecl(Context.getTranslationUnitDecl());

        // Create an output file to write the updated code
        FileID id = rewriter.getSourceMgr().getMainFileID();
        string filename = "/tmp/metadata_" + basename(rewriter.getSourceMgr().getFilename(rewriter.getSourceMgr().getLocForStartOfFile(id)).str());
        std::error_code OutErrorInfo;
        std::error_code ok;
        llvm::raw_fd_ostream outFile(llvm::StringRef(filename),
            OutErrorInfo, llvm::sys::fs::OF_None);
        if (OutErrorInfo == ok) {
            const RewriteBuffer *RewriteBuf = rewriter.getRewriteBufferFor(id);
            outFile << std::string(RewriteBuf->begin(), RewriteBuf->end());
            errs() << "Output file created - " << filename << "\n";
        } else {
            llvm::errs() << "Could not create file\n";
        }
    }
};

// 2) Create the AST Consumer
class PluginRenameAction : public PluginASTAction {
protected:
    /*
    unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) {
        // Create a single ASTConsumer. Calls RenameAstConsumer
        return make_unique<RenameASTConsumer>(&CI);
    }
    */
 
    bool ParseArgs(const CompilerInstance &CI, const vector<string> &args) {
        // 1) Copy the args vector to the global_args vector 
        global_args = args;     

        return true;
    }

    unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) {
        // Create a single ASTConsumer. Calls RenameAstConsumer
        return make_unique<RenameASTConsumer>(&CI);
    }



};

string getMetadataLines(vector<string> args){

    if(args.size() > MAX_ARGUMENTS){
        cout << "ERROR - La cantidad de argumentos pasado como archivo " << args.size() << " es mayor al valor permitido: " << MAX_ARGUMENTS << "\n", 
        exit(EXIT_FAILURE);
    }

    payload_lines* pay_lines_array = new payload_lines[args.size()]();
    payloadhdr_lines* payhdr_lines_array = new payloadhdr_lines[args.size()]();

    for (int z = 0; z < args.size(); z++) {

        cout << "Args[" << z << "] = " << args[z] << "\n";  // Print plugin arguments

        int fdescriptor_input;

        fdescriptor_input = open(args[z].c_str(), O_RDONLY);
        if (fdescriptor_input == -1) {
            cout << "Error file descriptor de .bin\n";
            exit(EXIT_FAILURE);
        }

        // Get the stat of the file
        struct stat stbuf;

        if ((fstat(fdescriptor_input, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode))) {
            cout << "Error stat del file .bin\n";
            exit(EXIT_FAILURE);
        }    

        // Get the file size
        off_t file_size;
        file_size = stbuf.st_size;

        // Close the open file descriptor
        close(fdescriptor_input);    

        // 2) Open file in binary mode 
        fstream rawbin_fp;

        rawbin_fp.open(args[z].c_str(), ios::in | ios::binary);
        if(rawbin_fp.is_open() == false){
            cout << "Error al leer archivo rawbin_fp\n";
            exit(EXIT_FAILURE);
        }

        // 3) Create the uint8_t buffer for the read bytes
        uint8_t* buffer;
        buffer = new uint8_t [file_size]();

        // 4) Read bytes from the .bin file
        rawbin_fp.read((char*) buffer, file_size);
        if(rawbin_fp.rdstate()){
            cout << "Error al leer archivo rawbin_fp hacia buffer - read()\n";
            exit(EXIT_FAILURE);       
        }

        // Get read bytes from the last read operation
        size_t bytes_read;
        bytes_read = rawbin_fp.gcount();
        if(bytes_read != file_size){
            cout << "READ - Bytes_read: " << bytes_read << " es distinto a file_size: " << file_size << "\n";
            exit(EXIT_FAILURE);    
        }

        // 5) Close the opened file
        rawbin_fp.close();

        // 6-1) Check file size is eq or less than BINPAYLOAD_MAXSIZE
        if(file_size > BINPAYLOAD_MAXSIZE){
            cout << "Error al abrir archivo para leer - size mayor que BINPAYLOAD_MAXSIZE - rawbin_fp\n";
            exit(EXIT_FAILURE);        
        }

        // 6-2) Print the read file as ASCII
        //cout << "\n\tDatos leidos en ASCII - sizeof(uint8_t): \n\t";
        
        //for(int i = 0; i < file_size; i++){
        //    cout << buffer[i];
        //}
        //cout << "\n";

        // 6-3) Print the read file as Canonical hex (two hex digits per byte) (same as hexdump -C)
        //cout << "\n\tCanonical hex - (two hex digits per byte): \n\t";

        //for(int i = 0; i < file_size; i++){
        //    cout << hex << uppercase << static_cast<unsigned int>(buffer[i]) << " ";
        //}
        //cout << "\n";  

        //// 7) Iterate over array containing lines extracted

        uint8_t* payload_body;
        payload_body = new uint8_t [file_size]();        
        if (payload_body == NULL){
            cout << "error malloc payload_body\n";
            exit(EXIT_FAILURE);
        }

        // 8) Copy buffer to payload_body
        memcpy(payload_body, buffer, file_size);

        // 9) Create string for the payload_Binary line
        string payload_line;

        // 10) Call payload_Binary_fill()
        payload_Binary_fill(args.size(), payload_body, payload_line, file_size);
        //cout << "payload_line: " << payload_line << "\n";   // COUT

        // 11) Add line to payload_lines array
        addPayload_line(payload_line, &pay_lines_array[z]);     // CHANGE INDEX TO ITERATOR

        //////////////////
        int function_num_hardcoded = 1;                         // HARDCODED BINARY PAYLOAD FUNCTION NUMBER
        string function_num_string = to_string(function_num_hardcoded);
        /////////////////

        // 12) Add function number to payloadhdr_lines array
        create_addPayloadHdr_line(function_num_string, &payhdr_lines_array[z]);     // CHANGE INDEX TO ITERATOR

    }

    // 13) Iterate and obtain full payload lines
    string payloads_line_complete;

    fill_payloads_line(payloads_line_complete, args.size(), pay_lines_array);

    // 14) Iterate and obtain full payload_Hdr lines
    string payloadsHeader_line_complete;

    fill_payloadHeaders_line(payloadsHeader_line_complete, args.size(), payhdr_lines_array);

    // 15) Obtain full metadata_Hdr line
    string payloadsMetadataHdr_line_complete;

    string argc_string = to_string(args.size());

    fill_payloadMetadataHdr_line(payloadsMetadataHdr_line_complete, argc_string);

    // 16) Merge all lines
    string macro_complete;

    merge_lines(macro_complete, payloads_line_complete, payloadsHeader_line_complete, payloadsMetadataHdr_line_complete);        

    return macro_complete;
}



void merge_lines(string & macro_complete, string payloads_line_complete, string payloadsHeader_line_complete, string payloadsMetadataHdr_line_complete){

    macro_complete.append(payloadsMetadataHdr_line_complete); 
    macro_complete.append(payloads_line_complete);
    macro_complete.append(payloadsHeader_line_complete);

    //cout << "\nmacro_complete: " << macro_complete << "\n";     

}

void fill_payloadMetadataHdr_line(string & payloadsMetadataHdr_line_complete, string argc_string){

    payloadsMetadataHdr_line_complete.append("static Metadata_Hdr metadata_header __attribute__((__used__, __section__(\".metadata\"))) = {");
    payloadsMetadataHdr_line_complete.append(argc_string);
    payloadsMetadataHdr_line_complete.append(", sizeof(Payload_Hdr)};");

    //cout << "payloadsMetadataHdr_line_complete: " << payloadsMetadataHdr_line_complete << "\n";     

}



void fill_payloadHeaders_line(string & payloadsHeader_line_complete, int argc, struct payloadhdr_lines* payhdr_lines_array){

    payloadsHeader_line_complete.append("static Payload_Hdr payload_headers[] __attribute__((__used__, __section__(\".metadata\"))) = {");

    string index_string;

    for(int m = 0; m < argc; m++){
        payloadsHeader_line_complete.append("{");
        payloadsHeader_line_complete.append(payhdr_lines_array[m].string_function_num);
        payloadsHeader_line_complete.append(", sizeof(payloads[");

        index_string = to_string(m); 

        payloadsHeader_line_complete.append(index_string);
        payloadsHeader_line_complete.append("])}");
    
        if(m < argc - 1){
        payloadsHeader_line_complete.append(",");
        }
    }

    payloadsHeader_line_complete.append("}");

    //cout << "payloadsHeader_line_complete: " << payloadsHeader_line_complete << "\n"; 

}

void fill_payloads_line(string & payloads_line_complete, int argc, struct payload_lines* pay_lines_array){

    payloads_line_complete.append("static Payload_Binary payloads[] __attribute__((__used__, __section__(\".metadata\"), __aligned__(8))) = {");
    for(int k = 0; k < argc; k++){
        payloads_line_complete.append(pay_lines_array[k].line);
        if(k < argc - 1){
        payloads_line_complete.append(",");
        }
    }

    payloads_line_complete.append("};");

    //cout << "payloads_line_complete: " << payloads_line_complete << "\n";

}

void create_addPayloadHdr_line(string funct_number, struct payloadhdr_lines* payloadHdr_line_struct){

    payloadHdr_line_struct-> string_function_num = funct_number;

    //cout << "payloadHdr_line_struct-> string_function_num: " << payloadHdr_line_struct-> string_function_num << "\n";

}

void addPayload_line(string line, struct payload_lines* payload_line_struct){

    size_t line_length = line.length();

    payload_line_struct->line = line;

    //cout << "payload_line_struct->line: " << payload_line_struct->line << "\n";
} 


void payload_Binary_fill(int args_count, uint8_t* binary_data, string & filled, long file_size){

    //cout << "\t    // payload_Binary_fill() // count_args: " << args_count << "\n";
    //cout << "\t";
    //for(int j = 0; j < file_size; j++){   
    //    cout << hex << uppercase << static_cast<unsigned int>(binary_data[j]) << " "; 
    //}
    //cout << "\n";

    // Create strings for fields
    // Binary file size
    string file_size_string = to_string(file_size);

    // Canonical hex bytes as string

    string hex_bytes;
    stringstream stream;
    size_t count = 0;

    for(int i = 0; i < file_size; i++){
        stream << hex << uppercase << setw(2) << static_cast<unsigned int>(binary_data[i]);
    }

    //cout << "hex_bytes: " << hex_bytes << "\n";     // COUT

    hex_bytes = stream.str();

    stream.str(string());
    stream.clear();

    cout << resetiosflags(cout.flags()); // clears all flags - reset cout

    // Length of bytes as string 
    
    size_t bytes_char_length = hex_bytes.length();
    //cout << "bytes_char_length: " << bytes_char_length << "\n";     // COUT    

    if(bytes_char_length > BINPAYLOAD_MAXSIZE){
        cout << "Bytes como string (" << bytes_char_length << ") es mas grande que BINPAYLOAD_MAXSIZE - payload_Binary_fill\n";
        exit(EXIT_FAILURE);        
    }

    string bytes_char_len_string = to_string(bytes_char_length);
    //cout << "bytes_char_len_string: " << bytes_char_len_string << "\n";     // COUT 

    // CREATE THE FULL STRING

    string payloads_type_Binary;

    payloads_type_Binary.append("{");
    payloads_type_Binary.append(file_size_string);
    payloads_type_Binary.append(",");
    payloads_type_Binary.append(bytes_char_len_string);
    payloads_type_Binary.append(",");
    payloads_type_Binary.append("\"");
    payloads_type_Binary.append(hex_bytes);
    payloads_type_Binary.append("\"");
    payloads_type_Binary.append("}");

    //cout << "\npayloads_type_Binary: \n\t" << payloads_type_Binary << "\n";

    // Assign the line to the string passed by reference

    filled = payloads_type_Binary;

}



// 1) Init.
// Create an instance of PluginRenameAction
static FrontendPluginRegistry::Add<PluginRenameAction>
    X("InsertPayload_Plugin", "Insert metadata plugin");
