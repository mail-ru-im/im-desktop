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

				auto tokens =  _style.split(QRegExp("\\;"));

				for (auto iter_line = tokens.begin(); iter_line != tokens.end(); iter_line++)
				{
					if (iter_line != tokens.begin())
						result << ";";

					int pos = iter_line->indexOf(QRegExp("[\\-\\d]\\d*dip"));

					if (pos != -1)
					{
						result << iter_line->left(pos);
						QString tmp = iter_line->mid(pos, iter_line->right(pos).length());
						int size = dpi::scale(QVariant(tmp.left(tmp.indexOf("dip"))).toInt());
												
						result << QVariant(size).toString();
						result << "px";
					}
					else
					{
						pos = iter_line->indexOf("_100");
						if (pos != -1)
						{
							result << iter_line->left(pos);
							result << "_";

							result << QVariant(dpi::scale(1.0) * 100).toString();
							
							result << iter_line->mid(pos + 4, iter_line->length());
						}
						else
						{
							pos = iter_line->indexOf("/100/");
							if (pos != -1)
							{
								result << iter_line->left(pos);
								result << "/";
								result << QVariant(dpi::scale(1.0) * 100).toString();
								
								result << "/";
								result << iter_line->mid(pos + 5, iter_line->length());
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
					return "";
				}

				QString qss = file.readAll();
				if (qss.isEmpty())
				{
					return "";
				}

				QString out_string;
				QTextStream result(&out_string);

				QString font_family = "\"Segoe UI\"";
				if (QSysInfo().windowsVersion() < QSysInfo::WV_VISTA)
					font_family = "Arial";

				result << scale_style(qss.replace("%FONT_FAMILY%", font_family));

				return out_string;

			}
		}
	}
}

