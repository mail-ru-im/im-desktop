#include "stdafx.h"

#include "translit.h"

namespace
{
    std::map<QChar, QVector<QString>> makeTable()
    {
        std::map<QChar, QVector<QString>> result;

        result[QChar(1040)] = { QChar(97) };
        result[QChar(1041)] = { QChar(98) };
        result[QChar(1042)] = { QChar(118), QChar(119) };
        result[QChar(1043)] = { QChar(103) };
        result[QChar(1044)] = { QChar(100) };
        result[QChar(1045)] = { QChar(101), QChar(121) % QChar(101) };
        result[QChar(1025)] = { QChar(121) % QChar(111), QChar(105) % QChar(111), QChar(106) % QChar(111), QChar(101) };
        result[QChar(1046)] = { QChar(106), QChar(122), QChar(122) % QChar(104), QChar(103) };
        result[QChar(1047)] = { QChar(122), QChar(115) };
        result[QChar(1048)] = { QChar(105), QChar(121) };
        result[QChar(1049)] = { QChar(105), QChar(121) };
        result[QChar(1050)] = { QChar(107), QChar(99) % QChar(107), QChar(99) };
        result[QChar(1051)] = { QChar(108) };
        result[QChar(1052)] = { QChar(109) };
        result[QChar(1053)] = { QChar(110) };
        result[QChar(1054)] = { QChar(111) };
        result[QChar(1055)] = { QChar(112) };
        result[QChar(1056)] = { QChar(114) };
        result[QChar(1057)] = { QChar(115), QChar(99) };
        result[QChar(1058)] = { QChar(116) };
        result[QChar(1059)] = { QChar(117), QChar(121) };
        result[QChar(1060)] = { QChar(102), QChar(112) % QChar(104) };
        result[QChar(1061)] = { QChar(104), QChar(107) % QChar(104) };
        result[QChar(1062)] = { QChar(99), QChar(116) % QChar(115) };
        result[QChar(1063)] = { QChar(99) % QChar(104) };
        result[QChar(1064)] = { QChar(115) % QChar(104), QChar(115) % QChar(99) % QChar(104) };
        result[QChar(1065)] = { QChar(115) % QChar(99) % QChar(104), QChar(115) % QChar(104) % QChar(99) % QChar(104) };
        result[QChar(1066)] = { QChar(39), QChar(105), QChar(106), QChar(121) };
        result[QChar(1067)] = { QChar(105), QChar(121) };
        result[QChar(1068)] = { QChar(105), QChar(106), QChar(121), QString() };
        result[QChar(1069)] = { QChar(101) };
        result[QChar(1070)] = { QChar(117), QChar(106) % QChar(117), QChar(121) % QChar(117) };
        result[QChar(1071)] = { QChar(105) % QChar(97), QChar(106) % QChar(97), QChar(121) % QChar(97) };

        result[QChar(65)] = { QChar(1040) };
        result[QChar(66)] = { QChar(1041) };
        result[QChar(67)] = { QChar(1062), QChar(1050), QChar(1057) };
        result[QChar(68)] = { QChar(1044) };
        result[QChar(69)] = { QChar(1045), QChar(1069), QChar(1025) };
        result[QChar(70)] = { QChar(1060) };
        result[QChar(71)] = { QChar(1043), QChar(1044) % QChar(1046) };
        result[QChar(72)] = { QChar(1061) };
        result[QChar(73)] = { QChar(1048), QChar(1067), QChar(1068), QChar(1066), QChar(1049) };
        result[QChar(74)] = { QChar(1046), QChar(1066), QChar(1068), QChar(1044) % QChar(1046) };
        result[QChar(75)] = { QChar(1050) };
        result[QChar(76)] = { QChar(1051) };
        result[QChar(77)] = { QChar(1052) };
        result[QChar(78)] = { QChar(1053) };
        result[QChar(79)] = { QChar(1054) };
        result[QChar(80)] = { QChar(1055) };
        result[QChar(81)] = { QChar(1050) };
        result[QChar(82)] = { QChar(1056) };
        result[QChar(83)] = { QChar(1057) };
        result[QChar(84)] = { QChar(1058) };
        result[QChar(85)] = { QChar(1059), QChar(1070) };
        result[QChar(86)] = { QChar(1042) };
        result[QChar(87)] = { QChar(1042) };
        result[QChar(88)] = { QChar(1050) % QChar(1057), QChar(1048) % QChar(1050) % QChar(1057) };
        result[QChar(89)] = { QChar(1049), QChar(1059), QChar(1067), QChar(1048), QChar(1066), QChar(1068) };
        result[QChar(90)] = { QChar(1047), QChar(1046) };

        result[QChar(32)] = { QChar(32) };
        result[QChar(39)] = { QChar(1068), QChar(1066) };


        return result;
    }
}

namespace Translit
{
    std::vector<QVector<QString>> getPossibleStrings(const QString& text)
    {
        const static std::map<QChar, QVector<QString>> table = makeTable();

        int max = std::min(getMaxSearchTextLength(), text.length());
        std::vector<QVector<QString>> result(max);

        for (auto i = 0; i < max; ++i)
        {
            const auto it = table.find(text.at(i).toUpper());
            if (it == table.end())
                return std::vector<QVector<QString>>();

            result[i] = it->second;
        }

        return result;
    }
}
