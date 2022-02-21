#include "kigu/array.h"
#include "kigu/common.h"
#include "kigu/cstring.h"
#include "kigu/map.h"
#include "kigu/string.h"

//// deshi includes ////
#define DESHI_DISABLE_IMGUI
#include "core/assets.h"
#include "core/commands.h"
#include "core/console.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/imgui.h"
#include "core/renderer.h"
#include "core/storage.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/window.h"
#include "core/io.h"
#ifdef TRACY_ENABLE
#include "Tracy.hpp"
#endif
#include "core/profiling.h"
#include "math/math.h"
#include "tree-sitter/lib/include/tree_sitter/api.h"

#include "graphviz/gvc.h"

external TSLanguage *tree_sitter_cpp();

void printTSQueryError(const char* source, u32 erroff, TSQueryError err){


}



//source struct with methods to help navigate and manipulate source code by lines and columns 
struct Source {
    array<string> lines;



    //inserts a line before the specified line
    void add_line(u32 idx, const string& line){
        lines.insert(line, idx);
        
    }
    
    void replace_line(u32 idx, const string& line){
        
    }

    void remove_line(u32 idx){
        lines.remove(idx);
    }

    //returns a cstring pointing to 

    cstring at(u32 line, u32 column){
        return {lines[line].str+column, lines[line].count-column};
    }

};

void treeheaderrecur(TSNode node){
    static u32 id = 0;
    id++;
    if(UI::BeginHeader(toStr(ts_node_type(node), "##", id).str, UIHeaderFlags_NoIndentRight)){
        forI(ts_node_child_count(node)){
            treeheaderrecur(ts_node_child(node, i));    
        }
        if(UI::IsLastItemHovered()){
            UI::Continue("mainwin/codewin");{
                
            }UI::EndContinue();
        }

        UI::EndHeader();
    }
}

s32 main(){
    Assets::enforceDirectories();
	memory_init(Gigabytes(1), Gigabytes(1));
    Logger::Init(0, true);
    DeshTime->Init();
    DeshWindow->Init("deshi", 1280, 720, 100, 100);
	Render::Init();
    Storage::Init();
	UI::Init();
	Cmd::Init();
	DeshWindow->ShowWindow();
	Render::UseDefaultViewProjMatrix();

    File in = open_file("src/test.cpp", FileAccess_Read);
    char* data = read_file(in);
    FileReader reader = init_reader(in);
    string mutablecode = reader.raw;
    FileReader mutablereader = init_reader(mutablecode.str, mutablecode.count); 

    Source source(reader.raw);

    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_cpp());
    TSTree* tree = ts_parser_parse_string(parser, 0, reader.raw.str, reader.raw.count);
    PRINTLN(ts_node_string(ts_tree_root_node(tree)));

    TIMER_START(t_f);
	TIMER_START(fun);
    while(!DeshWindow->ShouldClose()){
        DeshWindow->Update();
		DeshTime->Update();
		DeshInput->Update();
		UI::Update();
		Render::Update();
		memory_clear_temp();
		DeshTime->frameTime = TIMER_END(t_f); TIMER_RESET(t_f);


        // Interactive Preprocessor

        //TODO comment all this ugly stuff
        //TODO move this somewhere else

        using namespace UI;
        SetNextWindowSize(DeshWinSize);
        SetNextWindowPos(vec2::ZERO);
        Begin("mainwin", UIWindowFlags_NoInteract);{
            array<UIItem*> lines; 
            array<UIItem*> mutatedlines;

            PushColor(UIStyleCol_WindowBg, color(20,20,20));
            PushColor(UIStyleCol_Border, color(75,75,75));
            SetNextWindowSize(vec2((GetMarginedRight()-GetMarginedLeft())*0.5, (GetMarginedBottom()-GetMarginedTop())*0.5));
            UIItem* codewinitem = 0;
            BeginChild("codewin", vec2::ZERO);{
                
                for(cstring& line : reader.lines){
                    Text(line, UITextFlags_NoWrap);
                    lines.add(GetLastItem());
                }
            }EndChild();
            codewinitem=GetLastItem();
            SetWinCursor(codewinitem->position.yAdd(codewinitem->size.y));
            SetNextWindowSize(vec2((GetMarginedRight()-GetMarginedLeft())*0.5, (GetMarginedBottom()-GetMarginedTop())*0.5));
            BeginChild("mutatedcodewin",vec2::ZERO);{
                for(cstring& line : mutablereader.lines){
                    Text(line, UITextFlags_NoWrap);
                    mutatedlines.add(GetLastItem());
                }
            }EndChild();
           
            SetWinCursor(codewinitem->position.xAdd(codewinitem->size.x));            
            SetNextWindowSize(vec2((GetMarginedRight()-GetMarginedLeft())*0.5, MAX_F32));
            BeginChild("infowin", vec2::ZERO);{
                PushVar(UIStyleVar_WindowMargins, vec2::ZERO);
                BeginTabBar("infowintabbar", UITabBarFlags_NoIndent);{
                    if(BeginTab("Query")){
                        PushVar(UIStyleVar_WindowMargins, vec2(5,5));
                        SetNextWindowSize(MAX_F32,MAX_F32);
                        BeginChild("querywin", vec2::ZERO, UIWindowFlags_NoBorder);{
                            static TSQueryError qerror = TSQueryErrorNone;
                            static u32 qerroff = 0;
                            static array<TSNode> matches;

                            persist char buff[255];
                            persist char errbuff[255]; //stores the last buffer an error occured on 
                            SetNextItemSize(MAX_F32, 0);
                            if(InputText("queryinput", buff, 255, "enter query...", UIInputTextFlags_AnyChangeReturnsTrue)){
                                 TSQuery* q = ts_query_new(tree_sitter_cpp(), buff, strlen(buff), &qerroff, &qerror);
                                 if(q) {
                                    TSQueryCursor* cursor = ts_query_cursor_new();
                                    ts_query_cursor_exec(cursor, q, ts_tree_root_node(tree));
                                    TSQueryMatch match;
                                    matches.clear();
                                    while(ts_query_cursor_next_match(cursor, &match)){
                                        forI(match.capture_count){
                                            matches.add(match.captures[i].node);
                                        }
                                    }
                                 }
                                 else{
                                    memcpy(errbuff,buff,255);
                                 }
                            }

                            Separator(10); //----------------------------------------------------------------

                            //display query results

                            if(buff[0] && qerror || !matches.count){
                                BeginRow("centeralign09172", 1, GetStyle().fontHeight);
                                RowSetupColumnAlignments({{0.5,0.5}});
                                RowSetupColumnWidth(0, GetMarginedRight()-GetMarginedLeft());
                                switch(qerror){
                                    case TSQueryErrorSyntax:{
                                        Text("syntax error at: "); 
                                        Text(cstring{errbuff+qerroff-1, (strlen(errbuff)-qerroff)+1});
                                    }break;
                                    case TSQueryErrorNodeType:{
                                        Text("invalid node type at");
                                        Text(cstring{errbuff+qerroff-1, (strlen(errbuff)-qerroff)+1});
                                    }break;
                                    case TSQueryErrorField:{
                                        Text("invalid field at");
                                        Text(cstring{errbuff+qerroff-1, (strlen(errbuff)-qerroff)+1});
                                    }break;
                                    case TSQueryErrorCapture:{
                                        Text("invalid capture at");
                                        Text(cstring{errbuff+qerroff-1, (strlen(errbuff)-qerroff)+1});
                                    }break;
                                    case TSQueryErrorStructure:{
                                        Text("structure error at");
                                        Text(cstring{errbuff+qerroff-1, (strlen(errbuff)-qerroff)+1});
                                    }break;
                                    case TSQueryErrorLanguage:{
                                        Text("language error at");
                                        Text(cstring{errbuff+qerroff-1, (strlen(errbuff)-qerroff)+1});
                                    }break;
                                    case TSQueryErrorNone:{
                                        Text("no captures found");
                                        Text("did you forget to append a group with @identifier ?");
                                    }break;
                                }
                                EndRow();
                            }
                            else if(matches.count){
                                for(TSNode& n : matches){
                                    
                                    Text(ts_node_type(n));
                                    
                                    TSPoint start = ts_node_start_point(n);
                                    TSPoint end = ts_node_end_point(n);
                                    bool hovered=IsLastItemHovered();
                                    Continue("mainwin/codewin");{
                                        PushLayer(GetCurrentLayer()-1);
                                        if(start.row==end.row){
                                            vec2 scharpos = CalcCharPosition(reader.lines[start.row], start.column);
                                            vec2 echarpos = CalcCharPosition(reader.lines[start.row], end.column);
                                            RectFilled(lines[start.row]->position+scharpos, vec2(echarpos.x-scharpos.x,GetStyle().fontHeight), (hovered?color(50,144,75,255):color(50,75,75)));
                                        }
                                        else{
                                            for(u32 i = start.row; i <= end.row; i++){
                                                if(i==start.row){
                                                    vec2 scharpos = CalcCharPosition(reader.lines[start.row], start.column);
                                                    RectFilled(lines[start.row]->position+scharpos, vec2((lines[start.row]->position.x+lines[start.row]->size.x)-scharpos.x,GetStyle().fontHeight), (hovered?color(50,144,75,255):color(50,75,75)));
                                                }
                                                else if(i==end.row){
                                                    vec2 echarpos = CalcCharPosition(reader.lines[start.row], end.column);
                                                    RectFilled(lines[end.row]->position, vec2(echarpos.x, GetStyle().fontHeight),(hovered?color(50,144,75,255):color(50,75,75)));
                                                }
                                                else{
                                                    RectFilled(lines[i]->position, lines[i]->size, (hovered?color(50,144,75,255):color(50,75,75)));
                                                }
                                            }
                                        }
                                        PopLayer();
                                    }EndContinue();
                                   
                                }
                            }

                        }EndChild();
                        PopVar();
                        EndTab();
                    }
                    if(BeginTab("Syntax Tree")){
                        u32 id = 0;
                        auto recur = [&](TSNode node, auto&& recur)->void{
                            id++;
                            if(UI::BeginHeader(toStr(ts_node_type(node), "##", id).str, UIHeaderFlags_NoIndentRight)){
                                bool hovered = UI::IsLastItemHovered();
                                forI(ts_node_named_child_count(node)){
                                    recur(ts_node_named_child(node, i), recur);   
                                }
                                if(hovered){
                                    TSPoint start = ts_node_start_point(node);
                                    TSPoint end = ts_node_end_point(node);
                                    UI::Continue("mainwin/codewin");{
                                        PushLayer(GetCurrentLayer()-1);
                                        if(start.row==end.row){
                                            vec2 scharpos = CalcCharPosition(reader.lines[start.row], start.column);
                                            vec2 echarpos = CalcCharPosition(reader.lines[start.row], end.column);
                                            RectFilled(lines[start.row]->position+scharpos, vec2(echarpos.x-scharpos.x,GetStyle().fontHeight), (hovered?color(50,144,75,255):color(50,75,75)));
                                        }
                                        else{
                                            for(u32 i = start.row; i <= end.row; i++){
                                                if(i==start.row){
                                                    vec2 scharpos = CalcCharPosition(reader.lines[start.row], start.column);
                                                    RectFilled(lines[start.row]->position+scharpos, vec2((lines[start.row]->position.x+lines[start.row]->size.x)-scharpos.x,GetStyle().fontHeight), (hovered?color(50,144,75,255):color(50,75,75)));
                                                }
                                                else if(i==end.row){
                                                    vec2 echarpos = CalcCharPosition(reader.lines[start.row], end.column);
                                                    RectFilled(lines[end.row]->position, vec2(echarpos.x, GetStyle().fontHeight),(hovered?color(50,144,75,255):color(50,75,75)));
                                                }
                                                else{
                                                    RectFilled(lines[i]->position, lines[i]->size, (hovered?color(50,144,75,255):color(50,75,75)));
                                                }
                                            }
                                        }
                                        PopLayer();
                                    }UI::EndContinue();
                                }
                                UI::EndHeader();
                            }
                        };
                        recur(ts_tree_root_node(tree), recur);

                        EndTab();
                    }
                }EndTabBar();
                PopVar();
            }EndChild();
            PopColor(2);
            
        }UI::End();

        UI::ShowMetricsWindow();
    }


  
}