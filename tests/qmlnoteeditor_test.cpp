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

    void deleteAtEndMergesListItems()
    {
        QmlNoteEditor editor;
        editor.resize(500, 300);
        editor.load(QStringLiteral("- [ ] first\n- [x] second\n- [ ] third"), Note::Markdown);
        editor.show();
        QTest::qWait(30);

        auto *quick = editor.findChild<QQuickWidget *>();
        auto *root  = quick->rootObject();
        QTRY_COMPARE(root->property("editors").toList().size(), 3);
        auto *first = root->property("editors").toList().constFirst().value<QObject *>();
        QVERIFY(first);
        QVERIFY(first->setProperty("cursorPosition", first->property("length")));
        QVERIFY(QMetaObject::invokeMethod(first, "forceActiveFocus"));

        QTest::keyClick(quick, Qt::Key_Delete);

        QTRY_COMPARE(editor.model()->data(editor.model()->index(0), NoteBlockModel::ItemsRole).toStringList(),
                     QStringList({ "firstsecond", "third" }));
        QCOMPARE(editor.model()->data(editor.model()->index(0), NoteBlockModel::CheckedRole).toList(),
                 QVariantList({ false, false }));
        QTRY_COMPARE(root->property("activeEditor").value<QObject *>()->property("cursorPosition").toInt(), 5);
    }

    void tabsSelectedChecklistItems()
    {
        QmlNoteEditor editor;
        editor.resize(500, 300);
        editor.load(QStringLiteral("- [ ] one\n- [ ] two"), Note::Markdown);
        editor.show();
        QTest::qWait(30);
        auto *quick = editor.findChild<QQuickWidget *>();
        auto *root  = quick->rootObject();
        QVERIFY(root);
        QVariant geometryValue;
        QVERIFY(QMetaObject::invokeMethod(root, "editorGeometry", Q_RETURN_ARG(QVariant, geometryValue),
                                          Q_ARG(QVariant, 0)));
        const auto geometry = geometryValue.toMap();
        QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier,
                          QPoint(geometry["x"].toInt() + 8, geometry["y"].toInt() + geometry["height"].toInt() / 2));
        QVERIFY(QMetaObject::invokeMethod(root, "selectAllDocument"));
        QTest::keyClick(quick, Qt::Key_Tab);
        QTRY_COMPARE(editor.model()->data(editor.model()->index(0), NoteBlockModel::IndentsRole).toList(),
                     QVariantList({ 0, 1 }));
        QTest::keyClick(quick, Qt::Key_Tab, Qt::ShiftModifier);
        QTRY_COMPARE(editor.model()->data(editor.model()->index(0), NoteBlockModel::IndentsRole).toList(),
                     QVariantList({ 0, 0 }));
    }

    void listToolbarActionConvertsActiveList()
    {
        QmlNoteEditor editor;
        editor.resize(500, 300);
        editor.load(QStringLiteral("- [ ] one\n  - [ ] two"), Note::Markdown);
        editor.show();
        QTest::qWait(30);
        auto    *quick = editor.findChild<QQuickWidget *>();
        auto    *root  = quick->rootObject();
        QVariant geometryValue;
        QVERIFY(QMetaObject::invokeMethod(root, "editorGeometry", Q_RETURN_ARG(QVariant, geometryValue),
                                          Q_ARG(QVariant, 1)));
        const auto geometry = geometryValue.toMap();
        QTRY_VERIFY(!geometry.isEmpty());
        QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier,
                          QPoint(geometry["x"].toInt() + 8, geometry["y"].toInt() + geometry["height"].toInt() / 2));
        QTRY_VERIFY(root->property("activeEditor").value<QObject *>());
        auto *activeBefore = root->property("activeEditor").value<QObject *>();
        activeBefore->setProperty("cursorPosition", 1);

        editor.insertList(NoteBlockModel::NumberedList);
        QTRY_COMPARE(editor.model()->rowCount(), 1);
        QTRY_COMPARE(editor.model()->data(editor.model()->index(0), NoteBlockModel::ItemTypesRole).toList(),
                     QVariantList({ int(NoteBlockModel::CheckList), int(NoteBlockModel::NumberedList) }));
        const auto items = editor.model()->data(editor.model()->index(0), NoteBlockModel::ItemsRole).toStringList();
        QCOMPARE(items.size(), 2);
        QVERIFY(items[0].startsWith(QStringLiteral("one")));
        QVERIFY(items[1].startsWith(QStringLiteral("two")));
        QCOMPARE(editor.model()->data(editor.model()->index(0), NoteBlockModel::IndentsRole).toList(),
                 QVariantList({ 0, 1 }));
        QCOMPARE(root->property("activeEditor").value<QObject *>(), activeBefore);
        QCOMPARE(activeBefore->property("cursorPosition").toInt(), 1);
    }

    void listKeyboardShortcutsConvertActiveLevel()
    {
        QmlNoteEditor editor;
        editor.resize(500, 300);
        editor.load(QStringLiteral("1. one\n2. two"), Note::Markdown);
        editor.show();
        QTest::qWait(30);
        auto *quick = editor.findChild<QQuickWidget *>();
        auto *root  = quick->rootObject();
        QTRY_COMPARE(root->property("editors").toList().size(), 2);
        auto *first = root->property("editors").toList().constFirst().value<QObject *>();
        QVERIFY(first);
        QVERIFY(QMetaObject::invokeMethod(first, "forceActiveFocus"));

        QTest::keyClick(quick, Qt::Key_8, Qt::ControlModifier | Qt::ShiftModifier);
        QTRY_COMPARE(editor.model()->data(editor.model()->index(0), NoteBlockModel::ItemTypesRole).toList(),
                     QVariantList({ int(NoteBlockModel::BulletList), int(NoteBlockModel::BulletList) }));
        QTest::keyClick(quick, Qt::Key_9, Qt::ControlModifier | Qt::ShiftModifier);
        QTRY_COMPARE(editor.model()->data(editor.model()->index(0), NoteBlockModel::ItemTypesRole).toList(),
                     QVariantList({ int(NoteBlockModel::CheckList), int(NoteBlockModel::CheckList) }));
        QTest::keyClick(quick, Qt::Key_7, Qt::ControlModifier | Qt::ShiftModifier);
        QTRY_COMPARE(editor.model()->data(editor.model()->index(0), NoteBlockModel::ItemTypesRole).toList(),
                     QVariantList({ int(NoteBlockModel::NumberedList), int(NoteBlockModel::NumberedList) }));
    }

    void headingKeyboardShortcutsConvertTextBlock()
    {
        QmlNoteEditor editor;
        editor.resize(500, 300);
        editor.load(QStringLiteral("heading"), Note::Markdown);
        editor.show();
        QTest::qWait(30);
        auto *quick = editor.findChild<QQuickWidget *>();
        auto *root  = quick->rootObject();
        QTRY_COMPARE(root->property("editors").toList().size(), 1);
        auto *text = root->property("editors").toList().constFirst().value<QObject *>();
        QVERIFY(text);
        QVERIFY(QMetaObject::invokeMethod(text, "forceActiveFocus"));

        QTest::keyClick(quick, Qt::Key_2, Qt::ControlModifier);
        QTRY_COMPARE(editor.model()->data(editor.model()->index(0), NoteBlockModel::TypeRole).toInt(),
                     int(NoteBlockModel::Heading));
        QCOMPARE(editor.model()->data(editor.model()->index(0), NoteBlockModel::HeadingLevelRole).toInt(), 2);
        QCOMPARE(editor.contents(), QStringLiteral("## heading"));

        QTRY_COMPARE(root->property("editors").toList().size(), 1);
        auto *heading = root->property("editors").toList().constFirst().value<QObject *>();
        QVERIFY(heading);
        QVERIFY(QMetaObject::invokeMethod(heading, "forceActiveFocus"));
        QTest::keyClick(quick, Qt::Key_0, Qt::ControlModifier);
        QTRY_COMPARE(editor.model()->data(editor.model()->index(0), NoteBlockModel::TypeRole).toInt(),
                     int(NoteBlockModel::Text));
        QCOMPARE(editor.contents(), QStringLiteral("heading"));
    }

    void inlineFormattingShortcutsWrapSelection()
    {
        QmlNoteEditor editor;
        editor.resize(500, 300);
        editor.load(QStringLiteral("bold text"), Note::Markdown);
        editor.show();
        QTest::qWait(30);
        auto *quick = editor.findChild<QQuickWidget *>();
        auto *root  = quick->rootObject();
        QTRY_COMPARE(root->property("editors").toList().size(), 1);
        auto *text = root->property("editors").toList().constFirst().value<QObject *>();
        QVERIFY(text);
        QVERIFY(QMetaObject::invokeMethod(text, "forceActiveFocus"));
        QVERIFY(QMetaObject::invokeMethod(text, "select", Q_ARG(int, 0), Q_ARG(int, 4)));

        QTest::keyClick(quick, Qt::Key_B, Qt::ControlModifier);

        QTRY_COMPARE(editor.contents(), QStringLiteral("**bold** text"));
        QTest::keyClick(quick, Qt::Key_B, Qt::ControlModifier);
        QTRY_COMPARE(editor.contents(), QStringLiteral("bold text"));

        QMetaObject::invokeMethod(text, "select", Q_ARG(int, 0), Q_ARG(int, 4));
        QTest::keyClick(quick, Qt::Key_B, Qt::ControlModifier);
        QMetaObject::invokeMethod(text, "select", Q_ARG(int, 5), Q_ARG(int, 9));
        QTest::keyClick(quick, Qt::Key_I, Qt::ControlModifier);
        QMetaObject::invokeMethod(text, "select", Q_ARG(int, 0), Q_ARG(int, 9));
        auto *urlField = root->findChild<QObject *>(QStringLiteral("noteLinkUrlField"));
        QVERIFY(urlField);
        QTest::keyClick(quick, Qt::Key_K, Qt::ControlModifier);
        QTRY_VERIFY(urlField->property("activeFocus").toBool());
        QTest::keyClicks(quick, QStringLiteral("url"));
        QTest::keyClick(quick, Qt::Key_Return);
        QTRY_COMPARE(editor.contents(), QStringLiteral("[**bold** *text*](url)"));
    }

    void downLeavesHeadingForFollowingTextBlock()
    {
        QmlNoteEditor editor;
        editor.resize(500, 300);
        editor.load(QStringLiteral("## heading\n\nfollowing"), Note::Markdown);
        editor.show();
        QTest::qWait(30);
        auto *quick = editor.findChild<QQuickWidget *>();
        auto *root  = quick->rootObject();
        QTRY_COMPARE(root->property("editors").toList().size(), 2);
        auto *heading = root->property("editors").toList().constFirst().value<QObject *>();
        QVERIFY(heading);
        QVERIFY(QMetaObject::invokeMethod(heading, "forceActiveFocus"));
        heading->setProperty("cursorPosition", heading->property("length"));

        QTest::keyClick(quick, Qt::Key_Down);

        QTRY_COMPARE(root->property("activeEditor").value<QObject *>()->property("blockIndex").toInt(), 1);
        QCOMPARE(root->property("activeEditor").value<QObject *>()->property("cursorPosition").toInt(), 0);
    }

    void downFromLastHeadingDoesNotCreateTextBlock()
    {
        QmlNoteEditor editor;
        editor.resize(500, 300);
        editor.load(QStringLiteral("## heading"), Note::Markdown);
        editor.show();
        QTest::qWait(30);
        auto *quick = editor.findChild<QQuickWidget *>();
        auto *root  = quick->rootObject();
        QTRY_COMPARE(root->property("editors").toList().size(), 1);
        auto *heading = root->property("editors").toList().constFirst().value<QObject *>();
        QVERIFY(heading);
        QVERIFY(QMetaObject::invokeMethod(heading, "forceActiveFocus"));
        heading->setProperty("cursorPosition", heading->property("length"));

        QTest::keyClick(quick, Qt::Key_Down);

        QTest::qWait(20);
        QCOMPARE(editor.model()->rowCount(), 1);
        QCOMPARE(root->property("activeEditor").value<QObject *>()->property("blockIndex").toInt(), 0);
    }

    void downAtDocumentEndDoesNotAppendBlocks()
    {
        QmlNoteEditor editor;
        editor.resize(500, 300);
        editor.load(QStringLiteral("last"), Note::Markdown);
        editor.show();
        QTest::qWait(30);
        auto *quick = editor.findChild<QQuickWidget *>();
        auto *root  = quick->rootObject();
        QTRY_COMPARE(root->property("editors").toList().size(), 1);
        auto *text = root->property("editors").toList().constFirst().value<QObject *>();
        QVERIFY(text);
        QVERIFY(QMetaObject::invokeMethod(text, "forceActiveFocus"));
        text->setProperty("cursorPosition", text->property("length"));

        for (int press = 0; press < 6; ++press)
            QTest::keyClick(quick, Qt::Key_Down);
        QTest::qWait(20);
        QCOMPARE(editor.model()->rowCount(), 1);
    }

    void shiftSelectsAcrossEditors()
    {
        QmlNoteEditor editor;
        editor.resize(500, 350);
        editor.load(QStringLiteral("first\n\n- [ ] middle\n\nlast"), Note::Markdown);
        editor.show();
        QTest::qWait(30);
        auto *quick = editor.findChild<QQuickWidget *>();
        auto *root  = quick->rootObject();
        QVERIFY(root);
        QVariant geometryValue;
        QVERIFY(QMetaObject::invokeMethod(root, "editorGeometry", Q_RETURN_ARG(QVariant, geometryValue),
                                          Q_ARG(QVariant, 0)));
        const auto geometry = geometryValue.toMap();
        QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier,
                          QPoint(geometry["x"].toInt() + geometry["width"].toInt() - 8,
                                 geometry["y"].toInt() + geometry["height"].toInt() / 2));
        QTest::keyClick(quick, Qt::Key_End);
        QTest::keyClick(quick, Qt::Key_Left);
        QTest::keyClick(quick, Qt::Key_Right, Qt::ShiftModifier);
        QTest::keyClick(quick, Qt::Key_Right, Qt::ShiftModifier);
        QTest::keyClick(quick, Qt::Key_Right, Qt::ShiftModifier);
        auto selectedCount = [root]() {
            QVariant result;
            QMetaObject::invokeMethod(root, "selectedEditorCount", Q_RETURN_ARG(QVariant, result));
            return result.toInt();
        };
        QTRY_COMPARE(selectedCount(), 2);
    }

    void deleteRemovesSelectedListItems()
    {
        QmlNoteEditor editor;
        editor.resize(500, 350);
        editor.load(QStringLiteral("1. one\n2. two\n3. three"), Note::Markdown);
        editor.show();
        QTest::qWait(30);
        auto *quick = editor.findChild<QQuickWidget *>();
        auto *root  = quick->rootObject();
        QTRY_COMPARE(root->property("editors").toList().size(), 3);
        const auto editors = root->property("editors").toList();
        auto      *first   = editors[0].value<QObject *>();
        auto      *second  = editors[1].value<QObject *>();
        QVERIFY(first && second);
        const bool selectionApplied = QMetaObject::invokeMethod(
            root, "applyDocumentSelection", Q_ARG(QVariant, QVariant::fromValue(first)), Q_ARG(QVariant, 1),
            Q_ARG(QVariant, QVariant::fromValue(second)), Q_ARG(QVariant, 1));
        QVERIFY(selectionApplied);
        QTest::keyClick(quick, Qt::Key_Delete);
        QTRY_COMPARE(editor.model()->data(editor.model()->index(0), NoteBlockModel::ItemsRole).toStringList(),
                     QStringList({ "three" }));
    }

    void downLeavesLastStructuredBlock()
    {
        QmlNoteEditor listEditor;
        listEditor.resize(500, 350);
        listEditor.load(QStringLiteral("- one\n- two"), Note::Markdown);
        listEditor.show();
        QTest::qWait(30);
        auto    *listQuick = listEditor.findChild<QQuickWidget *>();
        auto    *listRoot  = listQuick->rootObject();
        QVariant geometryValue;
        QVERIFY(QMetaObject::invokeMethod(listRoot, "editorGeometry", Q_RETURN_ARG(QVariant, geometryValue),
                                          Q_ARG(QVariant, 1)));
        const auto geometry = geometryValue.toMap();
        QTest::mouseClick(listQuick, Qt::LeftButton, Qt::NoModifier,
                          QPoint(geometry["x"].toInt() + 8, geometry["y"].toInt() + geometry["height"].toInt() / 2));
        QTest::keyClick(listQuick, Qt::Key_End);
        QTest::keyClick(listQuick, Qt::Key_Down);
        QTRY_COMPARE(listEditor.model()->rowCount(), 2);
        QTRY_COMPARE(listRoot->property("activeEditor").value<QObject *>()->property("blockIndex").toInt(), 1);
        QCOMPARE(listEditor.model()->data(listEditor.model()->index(1), NoteBlockModel::TypeRole).toInt(),
                 int(NoteBlockModel::Text));
        auto *listActive = listRoot->property("activeEditor").value<QObject *>();
        QVERIFY(listActive);
        QTRY_VERIFY(listActive->property("activeFocus").toBool());
        QTest::keyClick(listQuick, Qt::Key_Up);
        QTRY_COMPARE(listRoot->property("activeEditor").value<QObject *>()->property("blockIndex").toInt(), 0);
    }

    void downLeavesLastTableRow()
    {
        QmlNoteEditor tableEditor;
        tableEditor.resize(500, 350);
        tableEditor.load(QStringLiteral("| Aaa | Bbb |\n| --- | --- |\n| Ccc | Ddd |"), Note::Markdown);
        tableEditor.show();
        QTest::qWait(30);
        auto *tableQuick        = tableEditor.findChild<QQuickWidget *>();
        auto *tableRoot         = tableQuick->rootObject();
        auto  tableCellGeometry = [tableRoot](int index) {
            QVariant result;
            QMetaObject::invokeMethod(tableRoot, "editorGeometry", Q_RETURN_ARG(QVariant, result),
                                       Q_ARG(QVariant, index));
            return result.toMap();
        };
        QTRY_VERIFY(!tableCellGeometry(3).isEmpty());
        const auto tableGeometry = tableCellGeometry(3);
        QTest::mouseClick(tableQuick, Qt::LeftButton, Qt::NoModifier,
                          QPoint(tableGeometry["x"].toInt() + tableGeometry["width"].toInt() / 2,
                                 tableGeometry["y"].toInt() + tableGeometry["height"].toInt() / 2));
        auto activeIndex = [tableRoot]() {
            QVariant result;
            QMetaObject::invokeMethod(tableRoot, "activeEditorIndex", Q_RETURN_ARG(QVariant, result));
            return result.toInt();
        };
        QTRY_COMPARE(activeIndex(), 3);
        QTest::keyClick(tableQuick, Qt::Key_End);
        QTest::keyClick(tableQuick, Qt::Key_Down);
        QTRY_COMPARE(tableRoot->property("activeEditor").value<QObject *>()->property("blockIndex").toInt(), 1);
        QTRY_COMPARE(tableEditor.model()->rowCount(), 2);
        auto *tableActive = tableRoot->property("activeEditor").value<QObject *>();
        QVERIFY(tableActive);
        QTRY_VERIFY(tableActive->property("activeFocus").toBool());
        QTest::keyClick(tableQuick, Qt::Key_Up);
        QTRY_COMPARE(tableRoot->property("activeEditor").value<QObject *>()->property("blockIndex").toInt(), 0);
    }

    void backspaceRemovesEmptyTableRow()
    {
        QmlNoteEditor editor;
        editor.resize(500, 350);
        editor.load(QStringLiteral("| A | B |\n| --- | --- |\n| one | two |"), Note::Markdown);
        editor.model()->insertTableRow(0, 2);
        editor.show();
        QTest::qWait(30);
        auto    *quick = editor.findChild<QQuickWidget *>();
        auto    *root  = quick->rootObject();
        QVariant geometryValue;
        QVERIFY(QMetaObject::invokeMethod(root, "editorGeometry", Q_RETURN_ARG(QVariant, geometryValue),
                                          Q_ARG(QVariant, 4)));
        const auto geometry = geometryValue.toMap();
        QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier,
                          QPoint(geometry["x"].toInt() + 8, geometry["y"].toInt() + geometry["height"].toInt() / 2));
        QTest::keyClick(quick, Qt::Key_Backspace);
        QTRY_COMPARE(editor.model()
                         ->data(editor.model()->index(0), NoteBlockModel::CellsRole)
                         .toMap()[QStringLiteral("values")]
                         .toStringList()
                         .size(),
                     4);
    }

    void insertsTableAndNumberedListBlocks()
    {
        QmlNoteEditor editor;
        editor.resize(500, 350);
        editor.load(QStringLiteral("text"), Note::Markdown);
        editor.show();
        QTest::qWait(30);

        editor.insertTable();
        QTRY_COMPARE(editor.model()->rowCount(), 2);
        QCOMPARE(editor.model()->data(editor.model()->index(1), NoteBlockModel::TypeRole).toInt(),
                 int(NoteBlockModel::Table));
        QCOMPARE(editor.model()
                     ->data(editor.model()->index(1), NoteBlockModel::CellsRole)
                     .toMap()[QStringLiteral("values")]
                     .toStringList()
                     .size(),
                 4);

        QTest::qWait(30);
        editor.insertList(NoteBlockModel::NumberedList);
        QTRY_COMPARE(editor.model()->rowCount(), 3);
        QList<int> insertedTypes;
        insertedTypes << editor.model()->data(editor.model()->index(1), NoteBlockModel::TypeRole).toInt()
                      << editor.model()->data(editor.model()->index(2), NoteBlockModel::TypeRole).toInt();
        QVERIFY(insertedTypes.contains(int(NoteBlockModel::Table)));
        QVERIFY(insertedTypes.contains(int(NoteBlockModel::NumberedList)));
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
