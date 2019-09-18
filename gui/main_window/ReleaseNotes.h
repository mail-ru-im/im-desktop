#pragma once

namespace Ui
{

namespace ReleaseNotes
{

class ReleaseNotesWidget : public QWidget
{
    Q_OBJECT

public:
    ReleaseNotesWidget(QWidget* _parent = nullptr);
};

void showReleaseNotes();

QString releaseNotesHash();

} // ReleaseNotes

} // Ui
