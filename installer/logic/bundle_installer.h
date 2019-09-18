#pragma once

namespace installer
{
    namespace logic
    {
        class BundleTask : public QRunnable
        {
        public:
            BundleTask(const bool _installHP, const bool _installSearch);

            void run() override;

        private:
            std::map<QString, QString> unpackBundle();
            void runSwitcher(const QString& _filePath);
            QString getUnpackPath(const QString& _fileName);


            bool installHP_;
            bool installSearch_;
        };

        void runBundle(const bool _installHP, const bool _installSearch);
    }
}