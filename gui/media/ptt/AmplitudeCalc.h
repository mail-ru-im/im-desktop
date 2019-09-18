#pragma once

namespace ptt
{
    class AmplitudeCalc
    {
    public:
        explicit AmplitudeCalc(const std::reference_wrapper<const QByteArray>& _data, size_t _offset, int _rate);

        [[nodiscard]] QVector<double> getSamples() const;

        void reset();

    private:
        bool newSamplesAvailable() const;

    private:
        using valueType = short;

        const std::reference_wrapper<const QByteArray> data_;
        const size_t offset_;
        mutable size_t pos_ = 0;
        const int rate_;
    };
}
