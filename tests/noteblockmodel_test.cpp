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

    void editsAreSerialized()
    {
        NoteBlockModel model;
        model.load(QStringLiteral("- [ ] first"), true);
        model.setChecked(0, 0, true);
        model.setListItem(0, 0, QStringLiteral("changed"));
        QCOMPARE(model.contents(), QStringLiteral("- [x] changed"));
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
