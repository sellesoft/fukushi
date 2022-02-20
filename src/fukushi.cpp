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
    FileReader reader = init_reader(in);

    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_cpp());
    TSTree* tree = ts_parser_parse_string(parser, 0, reader.raw.str, reader.raw.count);
    PRINTLN(ts_node_string(ts_tree_root_node(tree)));

    TIMER_START(t_f);
	TIMER_START(fun);
    //const char* query = ""
    //"(binary_expression)@capture"
    //"(function_definition)@capture";
    //u32 erroffset = 0;
    //TSQueryError qerror=TSQueryErrorNone;
    //TSQuery* q = ts_query_new(tree_sitter_cpp(), query, strlen(query), &erroffset, &qerror);
    //if(!q)printTSQueryError(query, erroffset, qerror);
    //TSQueryCursor* cursor = ts_query_cursor_new();
    //ts_query_cursor_exec(cursor, q, ts_tree_root_node(tree));
    //array<TSNode> matches;
    //TSQueryMatch* match = (TSQueryMatch*)memalloc(sizeof(TSQueryMatch));
    //PRINTLN("\n\n\n\n");
    //while(ts_query_cursor_next_match(cursor, match)){

    //    forI(match->capture_count){
    //        matches.add(match->captures[i].node);
    //    }
    //}
    //for(TSNode n : matches){
    //    PRINTLN(ts_node_string(n));
    //   // Log("",cstring{buffer + ts_node_start_byte(n), ts_node_end_byte(n)-ts_node_start_byte(n)});
    //}
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

            PushColor(UIStyleCol_WindowBg, color(20,20,20));
            PushColor(UIStyleCol_Border, color(75,75,75));
            SetNextWindowSize(vec2((GetMarginedRight()-GetMarginedLeft())*0.5, MAX_F32));
            BeginChild("codewin", vec2::ZERO);{
                PushLayer(GetCurrentLayer()+1);
                for(cstring& line : reader.lines){
                    Text(line, UITextFlags_NoWrap);
                    lines.add(GetLastItem());
                }
                PopLayer();
            }EndChild();
           
            SetWinCursor(GetLastItemPos().xAdd(GetLastItemSize().x));            
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
                            

                                    
                                        //vec2 lpos = line_positions[start.row];
                                        //vec2 charpos = CalcCharPosition(reader.lines[start.row], start.column);
                                        //Rect(lpos+charpos, vec2::ONE*30);
                                        PopLayer();
                                    }EndContinue();
                                   
                                }
                            }

                        }EndChild();
                        PopVar();
                        EndTab();
                    }
                    if(BeginTab("Syntax Tree")){
                        Text("placeholder");
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