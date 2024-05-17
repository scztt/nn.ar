// NNUGens.hpp

#pragma once

#include "NNModel.hpp"
#include "backend/backend.h"
#include "rt_circular_buffer.h"

#include "SC_PlugIn.hpp"
#undef Print

#include <semaphore>
#include <string>
#include <thread>

namespace NN {

using RingBuf = RingBufCtrl<float, float>;

enum Debug
{
    none = 0,
    attributes = 1,
    all = 2
};

/* class Timer { */
/*   std::chrono::high_resolution_clock::time_point start; */
/* public: */
/*   Timer() { reset(); } */

/*   void reset() { */
/*     start = std::chrono::high_resolution_clock::now(); */
/*   } */
/*   void print(const char* label) { */
/*     auto now = std::chrono::high_resolution_clock::now(); */
/*     std::cout << label */
/*       << std::chrono::duration_cast<std::chrono::milliseconds>(now - start) */
/*       << std::endl; */
/*   } */
/* }; */

class NNSetAttr
{
public:
    NNModelAttribute const* attr;
    // remember in0 indices
    int inputIdx;

    NNSetAttr(NNModelAttribute const* attr, int inputIdx, float initVal);

    // called in audio thread: check trig, update value and flag
    void update(Unit* unit, int nSamples);

    char const* getName() const { return attr->name.c_str(); }
    bool changed() const { return valUpdated; }
    // called before model_perform
    std::string getStrValue()
    {
        valUpdated = false;
        if (attr->type == NNAttributeType::typeBool)
            return value > 0 ? "true" : "false";
        else if (attr->type == NNAttributeType::typeInt)
            return std::to_string(static_cast<int>(value));
        return std::to_string(value);
    }

private:
    float lastTrig = 0;
    float value = 0;
    bool valUpdated = false;
};

class NN
{
public:
    NN(World* world,
       NNModelDesc const* modelDesc,
       NNModelMethod const* modelMethod,
       float* inModel,
       float* outModel,
       RingBuf* m_inBuffer,
       RingBuf* m_outBuffer,
       int bufferSize,
       int m_debug,
       int batches);

    ~NN();

    void bindModel();
    void warmupModel(int n_passes);

    RingBuf* m_inBuffer;
    RingBuf* m_outBuffer;
    float* m_inModel;
    float* m_outModel;
    NNModelDesc const* m_modelDesc;
    NNModelMethod const* m_method;
    World* mWorld;
    std::thread* m_compute_thread;
    std::binary_semaphore m_data_available_lock, m_result_available_lock;
    int m_inDim, m_outDim;
    int m_bufferSize, m_debug;
    int m_batches;
    std::vector<NNSetAttr> m_attributes;
    Backend m_model;
    bool m_should_stop_perform_thread;
    bool m_loaded;
    /* Timer timer; */
};

class NNUGen : public SCUnit
{
public:
    NNUGen();
    ~NNUGen();

    void next(int nSamples);
    void freeBuffers();
    void setupAttributes();

    NN* m_sharedData;

private:
    enum UGenInputs
    {
        modelIdx = 0,
        methodIdx,
        bufSize,
        warmup,
        debug,
        n_batches,
        inputs
    };
    void clearOutputs(int nSamples);
    bool allocBuffers();
    void updateAttributes();

    RingBuf* m_inBuffer;
    RingBuf* m_outBuffer;
    float* m_inModel;
    float* m_outModel;
    int m_inDim, m_outDim;
    int m_bufferSize, m_debug;
    int m_batches;
    bool m_useThread;
};

} // namespace NN
