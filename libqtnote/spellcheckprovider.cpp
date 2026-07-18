#include "spellcheckprovider.h"

#include <QRegularExpression>
#include <QTextCharFormat>

#include "notehighlighter.h"

namespace QtNote {
namespace {
    class ProviderSpellCheckExtension final : public SpellCheckExtension {
    public:
        explicit ProviderSpellCheckExtension(std::shared_ptr<SpellCheckProvider> provider) :
            provider_(std::move(provider))
        {
            expression_
                = QRegularExpression(QStringLiteral("[[:alpha:]]{2,}"), QRegularExpression::UseUnicodePropertiesOption);
        }

        void reset() override { }

        QStringList suggestions(const QString &word) const override { return provider_->suggestions(word); }
        void        addToDictionary(const QString &word) override { provider_->addToDictionary(word); }

        void highlight(NoteHighlighter *highlighter, const QString &text) override
        {
            QTextCharFormat format;
            format.setProperty(SpellCheckFormatProperty, true);

            auto matches = expression_.globalMatch(text);
            while (matches.hasNext()) {
                const auto match = matches.next();
                if (!provider_->isCorrect(match.captured()))
                    highlighter->addFormat(match.capturedStart(), match.capturedLength(), format);
            }
        }

    private:
        std::shared_ptr<SpellCheckProvider> provider_;
        QRegularExpression                  expression_;
    };
} // namespace

std::shared_ptr<SpellCheckExtension> makeSpellCheckExtension(const std::shared_ptr<SpellCheckProvider> &provider)
{
    if (!provider || !provider->isValid())
        return {};
    return std::make_shared<ProviderSpellCheckExtension>(provider);
}

} // namespace QtNote
