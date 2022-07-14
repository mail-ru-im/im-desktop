namespace launch
{
    class CommandLineParser
    {
        QString executable_;

        QString urlCommand_;

        bool isVersionCommand_ = false;

        bool developFlag_ = false;

    public:
        CommandLineParser(int _argc, char* _argv[]);

        ~CommandLineParser();

        bool isUrlCommand() const;
        bool isVersionCommand() const;
        bool hasDevelopFlag() const;

        const QString& getUrlCommand() const;

        const QString& getExecutable() const;
    };

    int main(int argc, char *argv[]);
}
