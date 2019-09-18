//
//  mac_translations.h
//  ICQ
//
//  Created by g.ulyanov on 14/06/16.
//  Copyright © 2016 Mail.RU. All rights reserved.
//

#ifndef mac_translations_h
#define mac_translations_h

namespace Utils
{
    class Translations
    {
    private:
        std::map<QString, QString> strings_;

    public:
        static const QString &Get(const QString &key);
        
    private:
        Translations();
        ~Translations() {}
        
        Translations(const Translations &);
        Translations(Translations &&);
        Translations &operator=(const Translations &);

    };
}

#endif /* mac_translations_h */
