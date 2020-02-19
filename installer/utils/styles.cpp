#include "stdafx.h"
#include "styles.h"


namespace installer
{
	namespace ui
	{
		namespace styles
		{
			QString scale_style(const QString& _style)
			{
				QString out_string;
				QTextStream result(&out_string);

				const auto tokens =  _style.split(QRegExp(ql1s("\\;")));

				for (auto iter_line = tokens.begin(); iter_line != tokens.end(); ++iter_line)
				{
					if (iter_line != tokens.begin())
						result << ";";

					int pos = iter_line->indexOf(QRegExp(ql1s("[\\-\\d]\\d*dip")));

					if (pos != -1)
					{
						result << iter_line->leftRef(pos);
						const auto tmp = iter_line->midRef(pos, iter_line->rightRef(pos).size());
						const int size = dpi::scale(tmp.left(tmp.indexOf(ql1s("dip"))).toInt());

						result << size;
						result << "px";
					}
					else
					{
						pos = iter_line->indexOf(ql1s("_100"));
						if (pos != -1)
						{
							result << iter_line->leftRef(pos);
							result << '_';

							result << dpi::scale(1.0) * 100;

							result << iter_line->midRef(pos + 4, iter_line->size());
						}
						else
						{
							pos = iter_line->indexOf(ql1s("/100/"));
							if (pos != -1)
							{
								result << iter_line->leftRef(pos);
								result << '/';
								result << dpi::scale(1.0) * 100;

								result << '/';
								result << iter_line->midRef(pos + 5, iter_line->size());
							}
							else
							{
								result << *iter_line;
							}
						}
					}
				}

				return out_string;
			}

			QString load_style(const QString& _file)
			{
				QFile file(_file);

				if (!file.open(QIODevice::ReadOnly))
				{
					assert(!"open style file error");
                    return QString();
				}

				QString qss = QString::fromUtf8(file.readAll());
                if (qss.isEmpty())
                    return QString();

				QString out_string;
				QTextStream result(&out_string);

				QString font_family = qsl("\"Segoe UI\"");
				if (QSysInfo().windowsVersion() < QSysInfo::WV_VISTA)
					font_family = qsl("Arial");

				result << scale_style(qss.replace(ql1s("%FONT_FAMILY%"), font_family));

				return out_string;

			}
		}
	}
}

