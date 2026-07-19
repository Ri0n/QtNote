#include <QtTest>

#include "noteblockmodel.h"

using namespace QtNote;

class NoteBlockModelTest : public QObject {
    Q_OBJECT

private slots:
    void plainTextIsOneBlock()
    {
        NoteBlockModel model;
        model.load(QStringLiteral("title\nbody"), false);
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.data(model.index(0), NoteBlockModel::TypeRole).toInt(), int(NoteBlockModel::Text));
        QCOMPARE(model.contents(), QStringLiteral("title\nbody"));
    }

    void parsesAndWritesGithubBlocks()
    {
        const QString  markdown = QStringLiteral("A [link](https://example.org)\n\n"
                                                  "- one\n- two\n\n"
                                                  "- [ ] todo\n- [x] done\n\n"
                                                  "| Name | Value |\n| --- | --- |\n| a | b |\n\n"
                                                  "![cat](media://cat)");
        NoteBlockModel model;
        model.load(markdown, true);
        QCOMPARE(model.rowCount(), 5);
        QCOMPARE(model.data(model.index(0), NoteBlockModel::TypeRole).toInt(), int(NoteBlockModel::Text));
        QCOMPARE(model.data(model.index(1), NoteBlockModel::TypeRole).toInt(), int(NoteBlockModel::BulletList));
        QCOMPARE(model.data(model.index(2), NoteBlockModel::TypeRole).toInt(), int(NoteBlockModel::CheckList));
        QCOMPARE(model.data(model.index(3), NoteBlockModel::TypeRole).toInt(), int(NoteBlockModel::Table));
        QCOMPARE(model.data(model.index(4), NoteBlockModel::TypeRole).toInt(), int(NoteBlockModel::Image));
        QVERIFY(model.contents().contains(QStringLiteral("- [x] done")));
        QVERIFY(model.contents().contains(QStringLiteral("| Name | Value |")));
        QVERIFY(model.contents().contains(QStringLiteral("[link](https://example.org)")));
    }

    void mergesAdjacentMarkdownParagraphs()
    {
        NoteBlockModel model;
        model.load(QStringLiteral("first paragraph\n\nsecond paragraph"), true);

        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.data(model.index(0), NoteBlockModel::TypeRole).toInt(), int(NoteBlockModel::Text));
        QCOMPARE(model.data(model.index(0), NoteBlockModel::TextRole).toString(),
                 QStringLiteral("first paragraph\n\nsecond paragraph"));
        QCOMPARE(model.contents(), QStringLiteral("first paragraph\n\nsecond paragraph"));
    }

    void keepsStructuralBoundariesBetweenTextSections()
    {
        NoteBlockModel model;
        model.load(QStringLiteral("before one\n\nbefore two\n\n- item\n\nafter one\n\nafter two"), true);

        QCOMPARE(model.rowCount(), 3);
        QCOMPARE(model.data(model.index(0), NoteBlockModel::TextRole).toString(),
                 QStringLiteral("before one\n\nbefore two"));
        QCOMPARE(model.data(model.index(1), NoteBlockModel::TypeRole).toInt(), int(NoteBlockModel::BulletList));
        QCOMPARE(model.data(model.index(2), NoteBlockModel::TextRole).toString(),
                 QStringLiteral("after one\n\nafter two"));
    }

    void textAfterBlankLineDoesNotJoinChecklistItem()
    {
        NoteBlockModel model;
        model.load(QStringLiteral("- [ ] task\n\nplain text"), true);
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.data(model.index(0), NoteBlockModel::TypeRole).toInt(), int(NoteBlockModel::CheckList));
        QCOMPARE(model.data(model.index(0), NoteBlockModel::ItemsRole).toStringList(), QStringList { "task" });
        QCOMPARE(model.data(model.index(1), NoteBlockModel::TypeRole).toInt(), int(NoteBlockModel::Text));
        QCOMPARE(model.data(model.index(1), NoteBlockModel::TextRole).toString(), QStringLiteral("plain text"));

        model.load(QStringLiteral("- [ ] task<br><br>\n\nplain text"), true);
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.data(model.index(0), NoteBlockModel::ItemsRole).toStringList(), QStringList { "task" });
        QCOMPARE(model.data(model.index(1), NoteBlockModel::TypeRole).toInt(), int(NoteBlockModel::Text));
        QCOMPARE(model.data(model.index(1), NoteBlockModel::TextRole).toString(), QStringLiteral("plain text"));
    }

    void editsAreSerialized()
    {
        NoteBlockModel model;
        model.load(QStringLiteral("- [ ] first"), true);
        model.setChecked(0, 0, true);
        model.setListItem(0, 0, QStringLiteral("changed"));
        QCOMPARE(model.contents(), QStringLiteral("- [x] changed"));
    }

    void coalescesAdjacentLinksCreatedAcrossFormatRuns()
    {
        NoteBlockModel model;
        model.load(QStringLiteral("text"), true);
        model.setBlockText(0, QStringLiteral("[Надежду ](url)[*Л*](url)[ебедев](url)[*у*](url)"));
        QCOMPARE(model.contents(), QStringLiteral("[Надежду *Л*ебедев*у*](url)"));
    }

    void editsTableStructure()
    {
        NoteBlockModel model;
        model.load(QStringLiteral("| A | B |\n| --- | --- |\n| one | two |"), true);
        QCOMPARE(model.rowCount(), 1);

        model.insertTableRow(0, 1);
        auto table = model.data(model.index(0), NoteBlockModel::CellsRole).toMap();
        QCOMPARE(table[QStringLiteral("columns")].toInt(), 2);
        QCOMPARE(table[QStringLiteral("values")].toStringList().size(), 6);

        model.insertTableColumn(0, 1);
        table = model.data(model.index(0), NoteBlockModel::CellsRole).toMap();
        QCOMPARE(table[QStringLiteral("columns")].toInt(), 3);
        QCOMPARE(table[QStringLiteral("values")].toStringList().size(), 9);

        model.removeTableRow(0, 1);
        model.removeTableColumn(0, 1);
        table = model.data(model.index(0), NoteBlockModel::CellsRole).toMap();
        QCOMPARE(table[QStringLiteral("columns")].toInt(), 2);
        QCOMPARE(table[QStringLiteral("values")].toStringList().size(), 4);
    }

    void tableLineBreaksUseGithubCompatibleHtml()
    {
        NoteBlockModel model;
        model.load(QStringLiteral("| A | B |\n| --- | --- |\n| one | two |"), true);
        model.setTableCell(0, 2, QStringLiteral("first\nsecond"));
        QVERIFY(model.contents().contains(QStringLiteral("first<br>second")));

        NoteBlockModel restored;
        restored.load(model.contents(), true);
        const auto table = restored.data(restored.index(0), NoteBlockModel::CellsRole).toMap();
        QCOMPARE(table[QStringLiteral("values")].toStringList().value(2), QStringLiteral("first\nsecond"));
    }

    void stripsTrailingListBreaksButKeepsInternalBreaks()
    {
        NoteBlockModel model;
        model.load(QStringLiteral("- [ ] first<br><br>"), true);
        QCOMPARE(model.data(model.index(0), NoteBlockModel::ItemsRole).toStringList().value(0),
                 QStringLiteral("first"));
        model.setListItem(0, 0, QStringLiteral("first\nsecond\n\n"));
        QCOMPARE(model.contents(), QStringLiteral("- [ ] first<br>second"));
    }

    void supportsNumberedAndIndentedLists()
    {
        NoteBlockModel numbered;
        numbered.load(QStringLiteral("1. one\n2. two"), true);
        QCOMPARE(numbered.data(numbered.index(0), NoteBlockModel::TypeRole).toInt(), int(NoteBlockModel::NumberedList));

        NoteBlockModel tasks;
        tasks.load(QStringLiteral("- [ ] one\n- [ ] two"), true);
        tasks.indentListItems(0, 0, 1, 1);
        QCOMPARE(tasks.data(tasks.index(0), NoteBlockModel::IndentsRole).toList(), QVariantList({ 0, 1 }));
        QVERIFY(tasks.contents().contains(QStringLiteral("  - [ ] two")));
        tasks.indentListItems(0, 0, 1, -1);
        QCOMPARE(tasks.data(tasks.index(0), NoteBlockModel::IndentsRole).toList(), QVariantList({ 0, 0 }));
    }

    void nestedTaskListSurvivesMarkdownRoundTrip()
    {
        NoteBlockModel model;
        model.load(QStringLiteral("- [ ] first\n- [ ] second\n- [ ] third"), true);
        model.indentListItems(0, 1, 1, 1);

        const QString  markdown = model.contents();
        NoteBlockModel restored;
        restored.load(markdown, true);

        QCOMPARE(restored.contents(), markdown);
        QCOMPARE(restored.rowCount(), 1);
        QCOMPARE(restored.data(restored.index(0), NoteBlockModel::ItemsRole).toStringList(),
                 QStringList({ "first", "second", "third" }));
        QCOMPARE(restored.data(restored.index(0), NoteBlockModel::IndentsRole).toList(), QVariantList({ 0, 1, 0 }));
    }

    void outdentedListItemAdoptsParentListType()
    {
        NoteBlockModel model;
        model.load(QStringLiteral("1. first\n2. child\n3. third"), true);
        model.indentListItems(0, 1, 1, 1);
        QVERIFY(model.convertListLevel(0, 1, NoteBlockModel::BulletList));
        QCOMPARE(model.data(model.index(0), NoteBlockModel::ItemTypesRole).toList(),
                 QVariantList({ int(NoteBlockModel::NumberedList), int(NoteBlockModel::BulletList),
                                int(NoteBlockModel::NumberedList) }));

        model.indentListItems(0, 1, 1, -1);

        QCOMPARE(model.data(model.index(0), NoteBlockModel::IndentsRole).toList(), QVariantList({ 0, 0, 0 }));
        QCOMPARE(model.data(model.index(0), NoteBlockModel::ItemTypesRole).toList(),
                 QVariantList({ int(NoteBlockModel::NumberedList), int(NoteBlockModel::NumberedList),
                                int(NoteBlockModel::NumberedList) }));
        QCOMPARE(model.contents(), QStringLiteral("1. first\n2. child\n3. third"));
    }

    void reindentedListItemRestoresNestedListType()
    {
        NoteBlockModel model;
        model.load(QStringLiteral("- [ ] parent\n- [ ] first\n- [ ] second\n- [ ] tail"), true);
        model.indentListItems(0, 1, 2, 1);
        QVERIFY(model.convertListLevel(0, 1, NoteBlockModel::BulletList));

        model.indentListItems(0, 1, 1, -1);
        QCOMPARE(model.data(model.index(0), NoteBlockModel::ItemTypesRole).toList().value(1).toInt(),
                 int(NoteBlockModel::CheckList));
        model.indentListItems(0, 1, 1, 1);

        QCOMPARE(model.data(model.index(0), NoteBlockModel::IndentsRole).toList(), QVariantList({ 0, 1, 1, 0 }));
        QCOMPARE(model.data(model.index(0), NoteBlockModel::ItemTypesRole).toList(),
                 QVariantList({ int(NoteBlockModel::CheckList), int(NoteBlockModel::BulletList),
                                int(NoteBlockModel::BulletList), int(NoteBlockModel::CheckList) }));
    }

    void taskListSurroundingNestedNumberedItemsStaysOneBlock()
    {
        NoteBlockModel model;
        model.load(QStringLiteral("- [ ] first\n- [ ] child\n- [ ] third"), true);
        model.indentListItems(0, 1, 1, 1);
        QVERIFY(model.convertListLevel(0, 1, NoteBlockModel::NumberedList));
        model.insertListItem(0, 2, QStringLiteral("new child"));

        const QString markdown = model.contents();
        QCOMPARE(markdown, QStringLiteral("- [ ] first\n    1. child\n    2. new child\n- [ ] third"));

        NoteBlockModel restored;
        restored.load(markdown, true);
        QCOMPARE(restored.rowCount(), 1);
        QCOMPARE(restored.data(restored.index(0), NoteBlockModel::ItemsRole).toStringList(),
                 QStringList({ "first", "child", "new child", "third" }));
        QCOMPARE(restored.data(restored.index(0), NoteBlockModel::IndentsRole).toList(), QVariantList({ 0, 1, 1, 0 }));
        QCOMPARE(restored.data(restored.index(0), NoteBlockModel::ItemTypesRole).toList(),
                 QVariantList({ int(NoteBlockModel::CheckList), int(NoteBlockModel::NumberedList),
                                int(NoteBlockModel::NumberedList), int(NoteBlockModel::CheckList) }));
        QCOMPARE(restored.contents(), markdown);
    }

    void preservesThreeLevelMixedListIndentation()
    {
        const QString  markdown = QStringLiteral("- [ ] 111\n"
                                                  "    1. ds\n"
                                                  "        - aaa bbb\n"
                                                  "    2. dsfgdg\n"
                                                  "- [ ] 32");
        NoteBlockModel model;
        model.load(markdown, true);

        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.data(model.index(0), NoteBlockModel::ItemsRole).toStringList(),
                 QStringList({ "111", "ds", "aaa bbb", "dsfgdg", "32" }));
        QCOMPARE(model.data(model.index(0), NoteBlockModel::IndentsRole).toList(), QVariantList({ 0, 1, 2, 1, 0 }));
        QCOMPARE(model.contents(), markdown);
    }

    void parsesSerializesAndSplitsHeadingBlocks()
    {
        NoteBlockModel parsed;
        parsed.load(QStringLiteral("# First\n\ntext\n\n### Third"), true);
        QCOMPARE(parsed.rowCount(), 3);
        QCOMPARE(parsed.data(parsed.index(0), NoteBlockModel::TypeRole).toInt(), int(NoteBlockModel::Heading));
        QCOMPARE(parsed.data(parsed.index(0), NoteBlockModel::HeadingLevelRole).toInt(), 1);
        QCOMPARE(parsed.data(parsed.index(2), NoteBlockModel::HeadingLevelRole).toInt(), 3);
        QCOMPARE(parsed.contents(), QStringLiteral("# First\n\ntext\n\n### Third"));

        NoteBlockModel converted;
        converted.load(QStringLiteral("before\n\ntarget\n\nafter"), true);
        QCOMPARE(converted.convertTextBlockToHeading(0, 10, 2), 1);
        QCOMPARE(converted.rowCount(), 3);
        QCOMPARE(converted.data(converted.index(1), NoteBlockModel::TypeRole).toInt(), int(NoteBlockModel::Heading));
        QCOMPARE(converted.data(converted.index(1), NoteBlockModel::TextRole).toString(), QStringLiteral("target"));
        QCOMPARE(converted.contents(), QStringLiteral("before\n\n## target\n\nafter"));
        QCOMPARE(converted.convertTextBlockToHeading(1, 0, 0), 1);
        QCOMPARE(converted.data(converted.index(1), NoteBlockModel::TypeRole).toInt(), int(NoteBlockModel::Text));
    }

    void insertsMinimalStructuredBlocks()
    {
        NoteBlockModel model;
        model.load(QStringLiteral("text"), true);
        model.insertTable(1);
        model.insertList(2, NoteBlockModel::CheckList);
        QCOMPARE(model.rowCount(), 3);
        const auto table = model.data(model.index(1), NoteBlockModel::CellsRole).toMap();
        QCOMPARE(table[QStringLiteral("columns")].toInt(), 2);
        QCOMPARE(table[QStringLiteral("values")].toStringList().size(), 4);
        QCOMPARE(model.data(model.index(2), NoteBlockModel::ItemsRole).toStringList(), QStringList { QString() });
    }

    void insertingAndConvertingListsPreservesIndentation()
    {
        NoteBlockModel model;
        model.load(QStringLiteral("- [ ] parent\n  - [ ] first\n  - [ ] second"), true);
        model.insertListItem(0, 2, QStringLiteral("new"));
        QCOMPARE(model.data(model.index(0), NoteBlockModel::IndentsRole).toList(), QVariantList({ 0, 1, 1, 1 }));

        QVERIFY(model.convertListLevel(0, 1, NoteBlockModel::NumberedList));
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.data(model.index(0), NoteBlockModel::ItemsRole).toStringList(),
                 QStringList({ "parent", "first", "new", "second" }));
        QCOMPARE(model.data(model.index(0), NoteBlockModel::IndentsRole).toList(), QVariantList({ 0, 1, 1, 1 }));
        QCOMPARE(model.data(model.index(0), NoteBlockModel::ItemTypesRole).toList(),
                 QVariantList({ int(NoteBlockModel::CheckList), int(NoteBlockModel::NumberedList),
                                int(NoteBlockModel::NumberedList), int(NoteBlockModel::NumberedList) }));
        QVERIFY(model.contents().contains(QStringLiteral("1. first")));
    }

    void removesStructuredRangesAtomically()
    {
        NoteBlockModel list;
        list.load(QStringLiteral("- one\n- two\n- three"), true);
        list.removeListItems(0, 0, 1);
        QCOMPARE(list.data(list.index(0), NoteBlockModel::ItemsRole).toStringList(), QStringList { "three" });

        NoteBlockModel table;
        table.load(QStringLiteral("| A | B |\n| --- | --- |\n| one | two |\n| three | four |"), true);
        table.removeTableRows(0, 0, 1);
        const auto cells = table.data(table.index(0), NoteBlockModel::CellsRole).toMap();
        QCOMPARE(cells[QStringLiteral("values")].toStringList(), QStringList({ "three", "four" }));
    }

    void previewUrlDoesNotChangeMarkdown()
    {
        NoteBlockModel model;
        model.load(QStringLiteral("![cat](media://one)"), true);
        model.setPreviewUrls({ { QStringLiteral("media://one"), QStringLiteral("image://qtnote-media/blob") } });
        QCOMPARE(model.data(model.index(0), NoteBlockModel::PreviewUrlRole).toString(),
                 QStringLiteral("image://qtnote-media/blob"));
        QCOMPARE(model.contents(), QStringLiteral("![cat](media://one)"));
    }
};

QTEST_MAIN(NoteBlockModelTest)
#include "noteblockmodel_test.moc"
