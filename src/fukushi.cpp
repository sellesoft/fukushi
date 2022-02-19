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
    GVC_t* gvc = gvContext();

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
    char* buffer = 0;
    FILE* in = fopen("src/test.cpp", "r");
    fseek(in, 0, SEEK_END);
    u64 sz = ftell(in);
    rewind(in);
    buffer = (char*)memalloc(sz);
    fread(buffer, sz, 1, in);

    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_cpp());
    
    Font* font = Storage::CreateFontFromFileTTF("STIXTwoText-Regular.otf", 100).second; 

    TIMER_START(t_f);
	TIMER_START(fun);
     TSTree* tree = ts_parser_parse_string(parser, 0, buffer, sz-1);                                                                                                                                                                                                                                                                   
    PRINTLN(ts_node_string(ts_tree_root_node(tree)));
    const char* query = ""
    "(binary_expression)@capture"
    "(function_definition)@capture";
    u32 erroffset = 0;
    TSQueryError qerror=TSQueryErrorNone;
    TSQuery* q = ts_query_new(tree_sitter_cpp(), query, strlen(query), &erroffset, &qerror);
    if(!q)printTSQueryError(query, erroffset, qerror);
    TSQueryCursor* cursor = ts_query_cursor_new();
    ts_query_cursor_exec(cursor, q, ts_tree_root_node(tree));
    array<TSNode> matches;
    TSQueryMatch* match = (TSQueryMatch*)memalloc(sizeof(TSQueryMatch));
    PRINTLN("\n\n\n\n");
    while(ts_query_cursor_next_match(cursor, match)){

        forI(match->capture_count){
            matches.add(match->captures[i].node);
        }
    }
    for(TSNode n : matches){
        PRINTLN(ts_node_string(n));
        Log("",cstring{buffer + ts_node_start_byte(n), ts_node_end_byte(n)-ts_node_start_byte(n)});
    }


    while(!DeshWindow->ShouldClose()){
        DeshWindow->Update();
		DeshTime->Update();
		DeshInput->Update();
		UI::Update();
		Render::Update();
		memory_clear_temp();
		DeshTime->frameTime = TIMER_END(t_f); TIMER_RESET(t_f);

        static b32 flip = 0;
        static s32 idx = 50;
        if(TIMER_END(fun)>250){
            if(idx>=50) flip=false;
            if(idx==0) flip=true;
            idx = Nudge(idx, (flip ? 50 : 0), 1);
            TIMER_RESET(fun);
        }
        UI::PushVar(UIStyleVar_FontHeight, 30);
        vec2 pos = UI::CalcCharPosition(cstring{buffer, sz}, idx);
        array<vec2> charpos;
        forI(sz){

        }
        UI::Begin("codewin");{
            UI::Text(buffer, UITextFlags_NoWrap);
            UI::Rect(UI::GetLastItemPos()+pos, UI::CalcTextSize(cstring{buffer+idx,1}));
            TSNode start = ts_tree_root_node(tree);
            array<TSNode> nstack;
            nstack.add(start);
            while(1){
                //for()
                
            }

        }UI::End();
        UI::PopVar();
    }


  
}