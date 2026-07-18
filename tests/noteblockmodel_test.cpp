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

    void editsAreSerialized()
    {
        NoteBlockModel model;
        model.load(QStringLiteral("- [ ] first"), true);
        model.setChecked(0, 0, true);
        model.setListItem(0, 0, QStringLiteral("changed"));
        QCOMPARE(model.contents(), QStringLiteral("- [x] changed"));
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
