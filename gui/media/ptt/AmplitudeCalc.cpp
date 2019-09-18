#include "stdafx.h"

#include "AmplitudeCalc.h"
#include "AudioRecorder2.h"
#include "AudioUtils.h"

namespace ptt
{
    AmplitudeCalc::AmplitudeCalc(const std::reference_wrapper<const QByteArray>& _data, size_t _offset, int _rate)
        : data_(_data)
        , offset_(_offset)
        , rate_(_rate)
    {
    }

    bool AmplitudeCalc::newSamplesAvailable() const
    {
        return (data_.get().size() - offset_) >= (pos_ + sampleBlockSizeForHist() * sizeof(valueType));
    }

    QVector<double> AmplitudeCalc::getSamples() const
    {
        QVector<double> res;
        while (newSamplesAvailable())
        {
            const auto first = reinterpret_cast<const short*>(data_.get().data() + offset_ + pos_);
            const auto last = first + sampleBlockSizeForHist();
            pos_ += sampleBlockSizeForHist() * sizeof(valueType);

            const auto maxAmpl = std::abs(*std::max_element(first, last, [](auto l, auto r) { return std::abs(l) < std::abs(r); })) * 1.5;

            res.push_back(double(maxAmpl) / std::numeric_limits<std::make_signed_t<valueType>>::max() / 2.0);
        }
        return res;
    }

    void AmplitudeCalc::reset()
    {
        pos_ = 0;
    }
}
