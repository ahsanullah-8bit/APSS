#pragma once

#include <memory>

#include "licensed/utility.h"
#include "apss.h"

class PaddleCls;
class PaddleDet;
class PaddleRec;

class PaddleOCREngine
{
public:
    explicit PaddleOCREngine();
    std::vector<PaddleOCR::OCRPredictResultList> predict(const MatList &batch);

private:
    std::unique_ptr<PaddleCls> m_cls;
    std::unique_ptr<PaddleDet> m_det;
    std::unique_ptr<PaddleRec> m_rec;
};
