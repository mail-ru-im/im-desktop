namespace launch
{
    class CommandLineParser
    {
        QString executable_;

        bool isUrlCommand_;
        QString urlCommand_;

        bool isVersionCommand_;

    public:

        CommandLineParser(int _argc, char* _argv[]);

        ~CommandLineParser();

        bool isUrlCommand() const;
        bool isVersionCommand() const;

        const QString& getUrlCommand() const;

        const QString& getExecutable() const;
    };

    int main(int argc, char *argv[]);
}
