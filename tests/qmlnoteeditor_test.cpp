#include <QClipboard>
#include <QQuickItem>
#include <QQuickTextDocument>
#include <QQuickWidget>
#include <QtTest>

#include "highlighterext.h"
#include "noteblockmodel.h"
#include "notehighlighter.h"
#include "qmlnoteeditor.h"

using namespace QtNote;

class CountingHighlighter : public SpellCheckExtension {
public:
    int     calls = 0;
    QString addedWord;
    void    reset() override { }
    void    highlight(NoteHighlighter *highlighter, const QString &text) override
    {
        ++calls;
        if (!text.isEmpty()) {
            QTextCharFormat format;
            format.setProperty(SpellCheckFormatProperty, true);
            highlighter->addFormat(0, text.size(), format);
        }
    }
    QStringList suggestions(const QString &word) const override { return { word + QStringLiteral("-fixed") }; }
    void        addToDictionary(const QString &word) override { addedWord = word; }
};

class QmlNoteEditorTest : public QObject {
    Q_OBJECT

private slots:
    void loadsQmlAndSwitchesFormats()
    {
        QmlNoteEditor editor;
        auto          quick = editor.findChild<QQuickWidget *>();
        QVERIFY(quick);
        QCOMPARE(quick->status(), QQuickWidget::Ready);

        editor.load(QStringLiteral("plain\ntext"), Note::PlainText);
        QCOMPARE(editor.model()->rowCount(), 1);
        QCOMPARE(editor.contents(), QStringLiteral("plain\ntext"));
        editor.insertText(QStringLiteral("dictated"));
        QCOMPARE(editor.contents(), QStringLiteral("plain\ntext dictated"));

        editor.load(QStringLiteral("- [ ] task\n\n| A | B |\n| --- | --- |\n| 1 | 2 |"), Note::Markdown);
        QCOMPARE(editor.model()->rowCount(), 2);
        QVERIFY(editor.isMarkdown());
    }

    void routesClipboardImages()
    {
        QmlNoteEditor editor;
        auto          quick = editor.findChild<QQuickWidget *>();
        QVERIFY(quick);
        QSignalSpy spy(&editor, &QmlNoteEditor::imagePasteRequested);
        QImage     image(3, 2, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::red);
        QGuiApplication::clipboard()->setImage(image);

        QTest::keyClick(quick, Qt::Key_V, Qt::ControlModifier);
        QCOMPARE(spy.size(), 1);
        QCOMPARE(qvariant_cast<QImage>(spy.first().first()).size(), QSize(3, 2));
    }

    void acceptsKeyboardInput()
    {
        QmlNoteEditor editor;
        editor.resize(400, 300);
        editor.load(QString(), Note::PlainText);
        editor.show();
        QTest::qWait(30);
        auto quick = editor.findChild<QQuickWidget *>();
        QVERIFY(quick);
        QVERIFY(quick->rootObject());
        QVERIFY(quick->rootObject()->width() > 0);
        QVERIFY(quick->rootObject()->height() > 0);
        editor.setFocus(Qt::OtherFocusReason);
        QTest::qWait(20);
        auto activeEditor = quick->rootObject()->property("activeEditor").value<QObject *>();
        QVERIFY(activeEditor);
        QVERIFY(activeEditor->property("activeFocus").toBool());

        // The final text block fills the viewport, so an empty note behaves as
        // one continuous editor even though it is structurally a block list.
        QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier, QPoint(30, 250));
        QTest::keyClicks(quick, QStringLiteral("typed"));
        QTRY_COMPARE(editor.contents(), QStringLiteral("typed"));
    }

    void invokesQmlDocumentHighlighters()
    {
        QmlNoteEditor editor;
        editor.resize(400, 300);
        editor.show();
        editor.load(QStringLiteral("misspelled"), Note::PlainText);
        auto extension = std::make_shared<CountingHighlighter>();
        editor.addHighlightExtension(extension, int(NoteHighlighter::SpellCheck));
        editor.rehighlight();
        editor.focusEditor();
        QTRY_VERIFY(extension->calls > 0);
        auto quick = editor.findChild<QQuickWidget *>();
        QVERIFY(quick);
        QTRY_VERIFY(quick->rootObject()->property("activeEditor").value<QObject *>());
        auto activeEditor = quick->rootObject()->property("activeEditor").value<QObject *>();
        auto document     = activeEditor->property("textDocument").value<QQuickTextDocument *>();
        QVERIFY(document);
        QCOMPARE(editor.spellCheckRanges(document).size(), 1);
        editor.setSpellCheckEnabled(false);
        QVERIFY(editor.spellCheckRanges(document).isEmpty());
        editor.setSpellCheckEnabled(true);
        QCOMPARE(editor.spellCheckRanges(document).size(), 1);
        QCOMPARE(editor.spellingSuggestions(QStringLiteral("wrong")), QStringList { QStringLiteral("wrong-fixed") });
        editor.addToSpellingDictionary(QStringLiteral("custom"));
        QCOMPARE(extension->addedWord, QStringLiteral("custom"));
    }

    void selectsCopiesAndCutsWholeBlockDocument()
    {
        QmlNoteEditor editor;
        editor.resize(500, 400);
        editor.load(QStringLiteral("first paragraph\n\nsecond paragraph"), Note::Markdown);
        editor.show();
        QTest::qWait(30);
        auto *quick = editor.findChild<QQuickWidget *>();
        QVERIFY(quick);
        auto *root = quick->rootObject();
        QVERIFY(root);

        QVERIFY(QMetaObject::invokeMethod(root, "selectAllDocument"));
        QVariant selected;
        QVERIFY(QMetaObject::invokeMethod(root, "selectedDocumentText", Q_RETURN_ARG(QVariant, selected)));
        QCOMPARE(selected.toString(), editor.contents());

        QVERIFY(QMetaObject::invokeMethod(root, "copyDocumentSelection"));
        QCOMPARE(QGuiApplication::clipboard()->text(), editor.contents());

        QVERIFY(QMetaObject::invokeMethod(root, "cutDocumentSelection"));
        QTRY_COMPARE(editor.contents(), QString());
    }

    void dragsSelectionAcrossTextBlocks()
    {
        QmlNoteEditor editor;
        editor.resize(500, 400);
        editor.load(QStringLiteral("first paragraph\n\nsecond paragraph"), Note::Markdown);
        editor.show();
        QTest::qWait(40);
        auto *quick = editor.findChild<QQuickWidget *>();
        QVERIFY(quick);
        auto *root = quick->rootObject();
        QVERIFY(root);

        auto geometry = [root](int index) {
            QVariant result;
            QMetaObject::invokeMethod(root, "editorGeometry", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, index));
            return result.toMap();
        };
        QTRY_VERIFY(!geometry(1).isEmpty());
        const auto   firstGeometry  = geometry(0);
        const auto   secondGeometry = geometry(1);
        const QPoint first(firstGeometry[QStringLiteral("x")].toInt() + 8,
                           firstGeometry[QStringLiteral("y")].toInt()
                               + firstGeometry[QStringLiteral("height")].toInt() / 2);
        const QPoint second(
            secondGeometry[QStringLiteral("x")].toInt() + secondGeometry[QStringLiteral("width")].toInt() - 8,
            secondGeometry[QStringLiteral("y")].toInt() + secondGeometry[QStringLiteral("height")].toInt() / 2);

        QTest::mousePress(quick, Qt::LeftButton, Qt::NoModifier, first);
        QTest::mouseMove(quick, second, 30);
        QTest::mouseRelease(quick, Qt::LeftButton, Qt::NoModifier, second);

        auto selectedEditorCount = [root]() {
            QVariant result;
            QMetaObject::invokeMethod(root, "selectedEditorCount", Q_RETURN_ARG(QVariant, result));
            return result.toInt();
        };
        QTRY_COMPARE(selectedEditorCount(), 2);
    }
};

QTEST_MAIN(QmlNoteEditorTest)
#include "qmlnoteeditor_test.moc"
