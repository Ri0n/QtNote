#include "notefragment.h"

#include <QTest>

using namespace QtNote;

class NoteFragmentTest : public QObject {
    Q_OBJECT

private slots:
    void roundTripsStructuredFragment();
    void rejectsInvalidInput();
};

void NoteFragmentTest::roundTripsStructuredFragment()
{
    NoteFragment fragment;
    fragment.kind         = NoteFragmentKind::BlockSequence;
    fragment.sourceFormat = NoteFragmentSourceFormat::Markdown;

    NoteFragmentBlock heading;
    heading.type         = NoteFragmentBlockType::Heading;
    heading.markdown     = QStringLiteral("**Release** notes");
    heading.headingLevel = 2;
    fragment.blocks.append(heading);

    NoteFragmentBlock list;
    list.type      = NoteFragmentBlockType::List;
    list.listItems = {
        { QStringLiteral("first"), 0, NoteFragmentListKind::Check, true },
        { QStringLiteral("*nested*"), 1, NoteFragmentListKind::Numbered, false },
    };
    fragment.blocks.append(list);

    NoteFragmentBlock table;
    table.type             = NoteFragmentBlockType::Table;
    table.table.rows       = 2;
    table.table.columns    = 2;
    table.table.headerRows = 1;
    table.table.markdownCells
        = { QStringLiteral("Name"), QStringLiteral("Status"), QStringLiteral("Alice"), QStringLiteral("**Ready**") };
    fragment.blocks.append(table);

    NoteFragmentBlock image;
    image.type            = NoteFragmentBlockType::Image;
    image.image.sourceUri = QStringLiteral("qtnote-media:/11111111-1111-1111-1111-111111111111/picture.png");
    image.image.alt       = QStringLiteral("Diagram");
    fragment.blocks.append(image);

    NoteFragmentMedia media;
    media.sourceUri              = image.image.sourceUri;
    media.reference.id           = QUuid(QStringLiteral("{11111111-1111-1111-1111-111111111111}"));
    media.reference.blobId       = QByteArray::fromHex("abcdef");
    media.reference.originalName = QStringLiteral("Picture.png");
    media.reference.portableName = QStringLiteral("picture.png");
    media.reference.mediaType    = QStringLiteral("image/png");
    media.reference.size         = 3;
    media.reference.checksum     = QByteArray::fromHex("012345");
    media.reference.remoteData.insert(QStringLiteral("revision"), 7);
    media.data = QByteArray("PNG");
    fragment.media.append(media);

    const NoteFragmentDecodeResult decoded = decodeNoteFragment(encodeNoteFragment(fragment));
    QVERIFY2(decoded, qPrintable(decoded.error));
    QCOMPARE(decoded.fragment.version, fragment.version);
    QCOMPARE(decoded.fragment.kind, fragment.kind);
    QCOMPARE(decoded.fragment.sourceFormat, fragment.sourceFormat);
    QCOMPARE(decoded.fragment.blocks.size(), 4);
    QCOMPARE(decoded.fragment.blocks.at(0).markdown, heading.markdown);
    QCOMPARE(decoded.fragment.blocks.at(0).headingLevel, 2);
    QCOMPARE(decoded.fragment.blocks.at(1).listItems.size(), 2);
    QCOMPARE(decoded.fragment.blocks.at(1).listItems.at(1).indent, 1);
    QCOMPARE(decoded.fragment.blocks.at(1).listItems.at(1).kind, NoteFragmentListKind::Numbered);
    QCOMPARE(decoded.fragment.blocks.at(2).table.markdownCells, table.table.markdownCells);
    QCOMPARE(decoded.fragment.blocks.at(3).image.alt, image.image.alt);
    QCOMPARE(decoded.fragment.media.size(), 1);
    QCOMPARE(decoded.fragment.media.at(0).reference.id, media.reference.id);
    QCOMPARE(decoded.fragment.media.at(0).reference.remoteData, media.reference.remoteData);
    QCOMPARE(decoded.fragment.media.at(0).data, media.data);
}

void NoteFragmentTest::rejectsInvalidInput()
{
    QCOMPARE(decodeNoteFragment(QByteArrayLiteral("not-cbor")).error, QStringLiteral("invalid CBOR fragment"));

    QCborMap wrongSchema;
    wrongSchema.insert(QStringLiteral("schema"), QStringLiteral("other"));
    QCOMPARE(decodeNoteFragment(QCborValue(wrongSchema).toCbor()).error, QStringLiteral("unknown fragment schema"));

    NoteFragment      invalidTable;
    NoteFragmentBlock table;
    table.type                = NoteFragmentBlockType::Table;
    table.table.rows          = 2;
    table.table.columns       = 2;
    table.table.markdownCells = { QStringLiteral("only one") };
    invalidTable.blocks.append(table);
    QVERIFY(!decodeNoteFragment(encodeNoteFragment(invalidTable)));

    NoteFragment      invalidList;
    NoteFragmentBlock list;
    list.type      = NoteFragmentBlockType::List;
    list.listItems = { { QStringLiteral("too deep"), 129, NoteFragmentListKind::Bullet, false } };
    invalidList.blocks.append(list);
    QVERIFY(!decodeNoteFragment(encodeNoteFragment(invalidList)));
}

QTEST_GUILESS_MAIN(NoteFragmentTest)

#include "notefragment_test.moc"
