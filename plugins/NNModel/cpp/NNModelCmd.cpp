#include "NNModelCmd.hpp"
#include "NNModel.hpp"
#include "SC_InterfaceTable.h"
#include "SC_PlugIn.hpp"

extern InterfaceTable* ft;
extern NN::NNModelDescLib gModels;

inline char* copyStrToBuf(char** buf, char const* str)
{
    char* res = strcpy(*buf, str);
    *buf += strlen(str) + 1;
    return res;
}

namespace NN::Cmd {

// /cmd /nn_set str str str
struct LoadCmdData
{
public:
    int id;
    char const* path;
    char const* filename;

    static LoadCmdData* alloc(sc_msg_iter* args, World* world = nullptr)
    {
        int id = args->geti(-1);
        char const* path = args->gets();
        char const* filename = args->gets("");

        if (path == 0)
        {
            Print("Error: nn_load needs a path to a .ts file\n");
            return nullptr;
        }

        size_t dataSize = sizeof(LoadCmdData) + strlen(path) + 1 + strlen(filename) + 1;

        LoadCmdData* cmdData = (LoadCmdData*)(world ? RTAlloc(world, dataSize) : NRTAlloc(dataSize));
        if (cmdData == nullptr)
        {
            Print("nn_load: msg data alloc failed.\n");
            return nullptr;
        }

        char* data = (char*)(cmdData + 1);
        cmdData->id = id;
        cmdData->path = copyStrToBuf(&data, path);
        cmdData->filename = copyStrToBuf(&data, filename);
        return cmdData;
    }

    LoadCmdData() = delete;
};

bool nn_load(World* world, void* inData)
{
    LoadCmdData* data = (LoadCmdData*)inData;
    int id = data->id;
    char const* path = data->path;
    char const* filename = data->filename;

    // Print("nn_load: idx %d path %s\n", id, path);
    auto model = (id == -1) ? gModels.load(path) : gModels.load(id, path);

    if (model != nullptr && strlen(filename) > 0)
    {
        model->dumpInfo(filename);
    }
    return true;
}

// /cmd /nn_query str
struct QueryCmdData
{
public:
    int modelIdx;
    char const* outFile;

    static QueryCmdData* alloc(sc_msg_iter* args, World* world = nullptr)
    {
        int modelIdx = args->geti(-1);
        char const* outFile = args->gets("");

        auto dataSize = sizeof(QueryCmdData) + strlen(outFile) + 1;
        QueryCmdData* cmdData = (QueryCmdData*)(world ? RTAlloc(world, dataSize) : NRTAlloc(dataSize));
        if (cmdData == nullptr)
        {
            Print("nn_query: alloc failed.\n");
            return nullptr;
        }
        cmdData->modelIdx = modelIdx;
        char* data = (char*)(cmdData + 1);
        cmdData->outFile = copyStrToBuf(&data, outFile);

        return cmdData;
    }

    QueryCmdData() = delete;
};

bool nn_query(World* world, void* inData)
{
    QueryCmdData* data = (QueryCmdData*)inData;
    int modelIdx = data->modelIdx;
    char const* outFile = data->outFile;
    bool writeToFile = strlen(outFile) > 0;
    if (modelIdx < 0)
    {
        if (writeToFile)
            gModels.dumpAllInfo(outFile);
        else
            gModels.printAllInfo();
        return true;
    }
    auto const model = gModels.get(static_cast<unsigned short>(modelIdx), true);
    if (model)
    {
        if (writeToFile)
            model->dumpInfo(outFile);
        else
            model->printInfo();
    }
    return true;
}

// /nn_unload i
struct UnloadCmdData
{
public:
    int id;

    static UnloadCmdData* alloc(sc_msg_iter* args, World* world = nullptr)
    {
        int id = args->geti(-1);

        size_t dataSize = sizeof(UnloadCmdData);
        UnloadCmdData*
            cmdData = (UnloadCmdData*)(world ? RTAlloc(world, dataSize) : NRTAlloc(dataSize));
        if (cmdData == nullptr)
        {
            Print("nn_unload: msg data alloc failed.\n");
            return nullptr;
        }
        cmdData->id = id;
        return cmdData;
    }

    UnloadCmdData() = delete;
};

bool nn_unload(World* world, void* inData)
{
    UnloadCmdData* data = (UnloadCmdData*)inData;
    int id = data->id;

    gModels.unload(id);

    return true;
}

// /cmd /nn_warmup int int
/* struct WarmupCmdData { */
/* public: */
/*   int modelIdx; */
/*   int methodIdx; */

/*   static WarmupCmdData* alloc(sc_msg_iter* args, World* world=nullptr) { */
/*     int modelIdx = args->geti(-1); */
/*     int methodIdx = args->geti(-1); */

/*     auto dataSize = sizeof(WarmupCmdData); */
/*     WarmupCmdData* cmdData = (WarmupCmdData*) (world ? RTAlloc(world, dataSize) : NRTAlloc(dataSize)); */
/*     if (cmdData == nullptr) { Print("nn_warmup: alloc failed.\n"); return nullptr; } */
/*     cmdData->modelIdx = modelIdx; */
/*     cmdData->methodIdx = methodIdx; */

/*     return cmdData; */
/*   } */

/*   WarmupCmdData() = delete; */
/* }; */

/* bool nn_warmup(World* world, void* inData) { */
/*   WarmupCmdData* data = (WarmupCmdData*)inData; */
/*   int modelIdx = data->modelIdx; */
/*   int methodIdx = data->methodIdx; */
/*   if (modelIdx < 0) { */
/*     Print("nn_warmup: invalid model index %d\n", modelIdx); */
/*     return true; */
/*   } */
/*   const auto model = gModels.get(static_cast<unsigned short>(modelIdx), true); */
/*   if (model) { */
/*     if (methodIdx < 0) { */
/*       // warmup all methods */
/*       for(auto method: model->m_methods) model->warmup_method(&method); */
/*     } else { */
/*       auto method = model->getMethod(methodIdx, true); */
/*       if (method) model->warmup_method(method); */
/*     } */
/*   } */
/*   return true; */
/* } */
void nrtFree(World*, void* data)
{
    NRTFree(data);
}

template <class CmdData, auto cmdFn>
void asyncCmd(World* world, void* inUserData, sc_msg_iter* args, void* replyAddr)
{
    char const* cmdName = ""; // used only in /done, we use /sync instead
    CmdData* data = CmdData::alloc(args, nullptr);
    if (data == nullptr)
        return;
    DoAsynchronousCommand(
        world,
        replyAddr,
        cmdName,
        data,
        cmdFn,   // stage2 is non real time
        nullptr, // stage3: RT (completion msg performed if true)
        nullptr, // stage4: NRT (sends /done if true)
        nrtFree,
        0,
        0
    );
}

void definePlugInCmds()
{
    DefinePlugInCmd("/nn_load", asyncCmd<LoadCmdData, nn_load>, nullptr);
    DefinePlugInCmd("/nn_query", asyncCmd<QueryCmdData, nn_query>, nullptr);
    DefinePlugInCmd("/nn_unload", asyncCmd<UnloadCmdData, nn_unload>, nullptr);
    /* DefinePlugInCmd("/nn_warmup", asyncCmd<WarmupCmdData, nn_warmup>, nullptr); */
}

} // namespace NN::Cmd
