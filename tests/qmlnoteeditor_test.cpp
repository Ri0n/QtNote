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
        const int callsBeforeRanges = extension->calls;
        QCOMPARE(editor.spellCheckRanges(document).size(), 1);
        QCOMPARE(extension->calls, callsBeforeRanges);
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

    void adjacentParagraphsUseOneTextEditor()
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
        auto geometry = [root](int index) {
            QVariant result;
            QMetaObject::invokeMethod(root, "editorGeometry", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, index));
            return result.toMap();
        };
        QTRY_VERIFY(!geometry(0).isEmpty());
        QVERIFY(geometry(1).isEmpty());
    }

    void editingTableCellKeepsTextDocuments()
    {
        QmlNoteEditor editor;
        editor.resize(500, 400);
        editor.load(QStringLiteral("| Name | Value |\n| --- | --- |\n| one | two |"), Note::Markdown);
        editor.show();
        QTest::qWait(30);

        auto *quick = editor.findChild<QQuickWidget *>();
        QVERIFY(quick);
        auto *root = quick->rootObject();
        QVERIFY(root);
        auto geometry = [root](int index) {
            QVariant result;
            QMetaObject::invokeMethod(root, "editorGeometry", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, index));
            return result.toMap();
        };
        QTRY_VERIFY(!geometry(3).isEmpty());
        const int registrations = root->property("editorRegistrations").toInt();
        QCOMPARE(registrations, 4);

        const auto   first = geometry(0);
        const QPoint position(first[QStringLiteral("x")].toInt() + first[QStringLiteral("width")].toInt() / 2,
                              first[QStringLiteral("y")].toInt() + first[QStringLiteral("height")].toInt() / 2);
        QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier, position);
        QTest::keyClicks(quick, QStringLiteral("x"));
        QTRY_VERIFY(editor.contents().contains(QLatin1Char('x')));
        QCOMPARE(root->property("editorRegistrations").toInt(), registrations);

        const auto   last = geometry(3);
        const QPoint lastPosition(last[QStringLiteral("x")].toInt() + last[QStringLiteral("width")].toInt() / 2,
                                  last[QStringLiteral("y")].toInt() + last[QStringLiteral("height")].toInt() / 2);
        QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier, lastPosition);
        QTest::keyClick(quick, Qt::Key_End);
        QTest::keyClick(quick, Qt::Key_Return, Qt::ShiftModifier);
        QTRY_COMPARE(geometry(2)[QStringLiteral("height")].toInt(), geometry(3)[QStringLiteral("height")].toInt());
        QCOMPARE(root->property("editorRegistrations").toInt(), registrations);
        QTRY_VERIFY(editor.contents().contains(QStringLiteral("<br>")));

        QTest::keyClick(quick, Qt::Key_End);
        QTest::keyClick(quick, Qt::Key_Return);
        QTRY_VERIFY(!geometry(5).isEmpty());
        const int registrationsAfterInsert = root->property("editorRegistrations").toInt();
        editor.model()->removeTableRow(0, 1);
        QTRY_VERIFY(geometry(4).isEmpty());
        QCOMPARE(root->property("editorRegistrations").toInt(), registrationsAfterInsert);
    }

    void editingChecklistItemKeepsTextDocumentsAndFocus()
    {
        QmlNoteEditor editor;
        editor.resize(500, 400);
        editor.load(QStringLiteral("- [ ] first item\n- [x] second item"), Note::Markdown);
        editor.show();
        QTest::qWait(30);

        auto *quick = editor.findChild<QQuickWidget *>();
        QVERIFY(quick);
        auto *root = quick->rootObject();
        QVERIFY(root);
        QVariant geometryValue;
        QVERIFY(QMetaObject::invokeMethod(root, "editorGeometry", Q_RETURN_ARG(QVariant, geometryValue),
                                          Q_ARG(QVariant, 0)));
        const auto geometry = geometryValue.toMap();
        QVERIFY(!geometry.isEmpty());
        const int registrations = root->property("editorRegistrations").toInt();
        QCOMPARE(registrations, 2);
        const QPoint position(geometry[QStringLiteral("x")].toInt() + geometry[QStringLiteral("width")].toInt() / 2,
                              geometry[QStringLiteral("y")].toInt() + geometry[QStringLiteral("height")].toInt() / 2);

        QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier, position);
        auto activeIndex = [root]() {
            QVariant result;
            QMetaObject::invokeMethod(root, "activeEditorIndex", Q_RETURN_ARG(QVariant, result));
            return result.toInt();
        };
        QTest::keyClick(quick, Qt::Key_Home);
        QTest::keyClick(quick, Qt::Key_Down);
        QTRY_COMPARE(activeIndex(), 1);
        QTest::keyClick(quick, Qt::Key_Up);
        QTRY_COMPARE(activeIndex(), 0);
        QTest::keyClicks(quick, QStringLiteral("xyz"));
        QTRY_VERIFY(editor.contents().contains(QStringLiteral("xyz")));
        QVERIFY(!editor.contents().contains(QStringLiteral("<br>")));
        QCOMPARE(root->property("editorRegistrations").toInt(), registrations);
        QVERIFY(root->property("activeEditor").value<QObject *>()->property("activeFocus").toBool());

        QTest::keyClick(quick, Qt::Key_Return);
        QTRY_COMPARE(editor.model()->data(editor.model()->index(0), NoteBlockModel::ItemsRole).toStringList().size(),
                     3);
        QTRY_COMPARE(root->property("editorRegistrations").toInt(), registrations + 1);
        QVERIFY(root->property("activeEditor").value<QObject *>()->property("activeFocus").toBool());

        QTest::keyClick(quick, Qt::Key_Home);
        QTest::keyClick(quick, Qt::Key_End, Qt::ShiftModifier);
        QTest::keyClick(quick, Qt::Key_Backspace);
        QTest::keyClick(quick, Qt::Key_Backspace);
        QTRY_COMPARE(editor.model()->data(editor.model()->index(0), NoteBlockModel::ItemsRole).toStringList().size(),
                     2);
        QVERIFY(root->property("activeEditor").value<QObject *>()->property("activeFocus").toBool());
    }

    void backspaceConvertsLastEmptyChecklistItemToText()
    {
        QmlNoteEditor editor;
        editor.resize(500, 300);
        editor.load(QStringLiteral("- [ ] "), Note::Markdown);
        editor.show();
        QTest::qWait(30);

        auto *quick = editor.findChild<QQuickWidget *>();
        QVERIFY(quick);
        auto *root = quick->rootObject();
        QVERIFY(root);
        QVariant geometryValue;
        QVERIFY(QMetaObject::invokeMethod(root, "editorGeometry", Q_RETURN_ARG(QVariant, geometryValue),
                                          Q_ARG(QVariant, 0)));
        const auto geometry = geometryValue.toMap();
        QTRY_VERIFY(!geometry.isEmpty());
        const QPoint position(geometry[QStringLiteral("x")].toInt() + 8,
                              geometry[QStringLiteral("y")].toInt() + geometry[QStringLiteral("height")].toInt() / 2);
        QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier, position);
        QTRY_VERIFY(root->property("activeEditor").value<QObject *>());
        QTest::keyClick(quick, Qt::Key_Backspace);

        QTRY_COMPARE(editor.model()->data(editor.model()->index(0), NoteBlockModel::TypeRole).toInt(),
                     int(NoteBlockModel::Text));
        QTRY_VERIFY(root->property("activeEditor").value<QObject *>());
        QVERIFY(root->property("activeEditor").value<QObject *>()->property("activeFocus").toBool());
        QTest::keyClicks(quick, QStringLiteral("plain"));
        QTRY_COMPARE(editor.contents(), QStringLiteral("plain"));
    }

    void selectingInsideTableCellAvoidsFullEditorScan()
    {
        QmlNoteEditor editor;
        editor.resize(600, 400);
        editor.load(QStringLiteral("| Name | Value |\n| --- | --- |\n| one | a fairly long value to select |"),
                    Note::Markdown);
        editor.show();
        QTest::qWait(30);

        auto *quick = editor.findChild<QQuickWidget *>();
        QVERIFY(quick);
        auto *root = quick->rootObject();
        QVERIFY(root);
        QVariant geometryValue;
        QVERIFY(QMetaObject::invokeMethod(root, "editorGeometry", Q_RETURN_ARG(QVariant, geometryValue),
                                          Q_ARG(QVariant, 3)));
        const auto geometry = geometryValue.toMap();
        QVERIFY(!geometry.isEmpty());
        const QPoint start(geometry[QStringLiteral("x")].toInt() + 8,
                           geometry[QStringLiteral("y")].toInt() + geometry[QStringLiteral("height")].toInt() / 2);
        const QPoint end(geometry[QStringLiteral("x")].toInt() + geometry[QStringLiteral("width")].toInt() - 8,
                         start.y());

        for (int iteration = 0; iteration < 20; ++iteration) {
            QVERIFY(QMetaObject::invokeMethod(root, "selectAllDocument"));
            QVERIFY(QMetaObject::invokeMethod(root, "clearDocumentSelection"));
        }

        QTest::mousePress(quick, Qt::LeftButton, Qt::NoModifier, start);
        const int fullSelectionPasses = root->property("fullSelectionPasses").toInt();
        QTest::mouseMove(quick, end, 30);
        QTest::mouseRelease(quick, Qt::LeftButton, Qt::NoModifier, end);

        QCOMPARE(root->property("fullSelectionPasses").toInt(), fullSelectionPasses);
    }

    void navigatesTableCellsWithArrowKeys()
    {
        QmlNoteEditor editor;
        editor.resize(500, 400);
        editor.load(QStringLiteral("| Aaa | Bbb |\n| --- | --- |\n| Ccc | Ddd |"), Note::Markdown);
        editor.show();
        QTest::qWait(30);

        auto *quick = editor.findChild<QQuickWidget *>();
        QVERIFY(quick);
        auto *root = quick->rootObject();
        QVERIFY(root);
        auto isBold = [root](int index) {
            QVariant result;
            QMetaObject::invokeMethod(root, "editorIsBold", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, index));
            return result.toBool();
        };
        QTRY_VERIFY(isBold(0));
        QVERIFY(isBold(1));
        QVERIFY(!isBold(2));
        QVERIFY(!isBold(3));
        auto activeIndex = [root]() {
            QVariant result;
            QMetaObject::invokeMethod(root, "activeEditorIndex", Q_RETURN_ARG(QVariant, result));
            return result.toInt();
        };
        QVariant geometryValue;
        QVERIFY(QMetaObject::invokeMethod(root, "editorGeometry", Q_RETURN_ARG(QVariant, geometryValue),
                                          Q_ARG(QVariant, 0)));
        const auto   geometry = geometryValue.toMap();
        const QPoint cellCenter(geometry[QStringLiteral("x")].toInt() + geometry[QStringLiteral("width")].toInt() / 2,
                                geometry[QStringLiteral("y")].toInt() + geometry[QStringLiteral("height")].toInt() / 2);
        QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier, cellCenter);
        QTRY_COMPARE(activeIndex(), 0);

        QTest::keyClick(quick, Qt::Key_Home);
        QTest::keyClick(quick, Qt::Key_Left);
        QTRY_COMPARE(activeIndex(), 0);
        QTest::keyClick(quick, Qt::Key_Right);
        QTRY_COMPARE(activeIndex(), 0);

        QTest::keyClick(quick, Qt::Key_End);
        QTest::keyClick(quick, Qt::Key_Right);
        QTRY_COMPARE(activeIndex(), 1);
        QTest::keyClick(quick, Qt::Key_Down);
        QTRY_COMPARE(activeIndex(), 3);
        QTest::keyClick(quick, Qt::Key_Home);
        QTest::keyClick(quick, Qt::Key_Left);
        QTRY_COMPARE(activeIndex(), 2);
        QTest::keyClick(quick, Qt::Key_Up);
        QTRY_COMPARE(activeIndex(), 0);
    }

    void dragsSelectionAcrossTextBlocks()
    {
        QmlNoteEditor editor;
        editor.resize(500, 400);
        editor.load(QStringLiteral("first paragraph\n\n- middle\n\nsecond paragraph"), Note::Markdown);
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
        QTRY_VERIFY(!geometry(2).isEmpty());
        const auto   firstGeometry  = geometry(0);
        const auto   secondGeometry = geometry(2);
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
        QTRY_COMPARE(selectedEditorCount(), 3);
    }
};

QTEST_MAIN(QmlNoteEditorTest)
#include "qmlnoteeditor_test.moc"
