#pragma once

#include <memory>

#include "apss.h"
#include "licensed/utility.h"
#include "wrappers/customallocator.h"
#include "paddlecls.h"
#include "paddledet.h"
#include "paddlerec.h"

class PaddleOCREngine
{
public:
    explicit PaddleOCREngine(std::shared_ptr<Ort::Env> env = {},
                             std::shared_ptr<CustomAllocator> allocator = {},
                             std::unique_ptr<PaddleDet> det = nullptr,
                             std::unique_ptr<PaddleCls> cls = nullptr,
                             std::unique_ptr<PaddleRec> rec = nullptr);
    std::vector<PaddleOCR::OCRPredictResultList> predict(const MatList &batch);

private:
    std::unique_ptr<PaddleCls> m_cls;
    std::unique_ptr<PaddleDet> m_det;
    std::unique_ptr<PaddleRec> m_rec;
};
