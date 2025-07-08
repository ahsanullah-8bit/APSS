#include "gtest/gtest.h"

// ODB headers for database interaction
#include <odb/core.hxx>
#include <odb/database.hxx>
#include <odb/transaction.hxx>
#include <odb/session.hxx>
#include <odb/schema-catalog.hxx> // For create_schema

// SQLite backend for ODB
#include <odb/sqlite/database.hxx>

// Qt headers
#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>

#include "db/db.h"
#include <db/recording>

// Define a test fixture for ODB operations
class RecordingOdbTest : public ::testing::Test
{
protected:
    std::unique_ptr<odb::database> db;

    void SetUp() override
    {
        db = std::make_unique<odb::sqlite::database>(":memory:", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
        odb::transaction t(db->begin());
        try {
            odb::schema_catalog::create_schema(*db);
            t.commit();
        } catch (const odb::exception& e) {
            qCritical() << "Failed to create schema:" << e.what();
            t.rollback();
            FAIL() << "Failed to create database schema: " << e.what();
        }
    }

    void TearDown() override
    {
        // Database destroyed on db unique_ptr scope exit
    }

    // Helper function to create a sample Recording object
    Recording createSampleRecording(const QString& idSuffix = "")
    {
        Recording rec;
        rec.setId("rec_id_" + idSuffix);
        rec.setCamera("CameraAlpha");
        rec.setPath("/recordings/cam_alpha_" + idSuffix + ".mp4");
        rec.setStartTime(QDateTime::currentDateTimeUtc().addSecs(-300));
        rec.setEndTime(QDateTime::currentDateTimeUtc());
        rec.setDuration(300.5f);
        rec.setMotion(1500);
        rec.setObjects(10);
        rec.setDBFS(-45);
        rec.setSegmentSize(10.2f);
        rec.setRegions(3);
        return rec;
    }

    // Helper function to compare two Recording objects for equality
    void expectRecordingsEqual(const Recording& expected, const Recording& actual)
    {
        EXPECT_EQ(actual.id(), expected.id());
        EXPECT_EQ(actual.camera(), expected.camera());
        EXPECT_EQ(actual.path(), expected.path());
        EXPECT_EQ(forceToUTC(actual.startTime()).toString(Qt::ISODateWithMs), forceToUTC(expected.startTime()).toString(Qt::ISODateWithMs));
        EXPECT_EQ(forceToUTC(actual.endTime()).toString(Qt::ISODateWithMs), forceToUTC(expected.endTime()).toString(Qt::ISODateWithMs));
        EXPECT_FLOAT_EQ(actual.duration(), expected.duration());
        EXPECT_EQ(actual.motion(), expected.motion());
        EXPECT_EQ(actual.objects(), expected.objects());
        EXPECT_EQ(actual.dBFS(), expected.dBFS());
        EXPECT_FLOAT_EQ(actual.segmentSize(), expected.segmentSize());
        EXPECT_EQ(actual.regions(), expected.regions());
    }
};

// Test Case 1: Persistence and Loading
TEST_F(RecordingOdbTest, PersistAndLoadRecording)
{
    Recording original_recording = createSampleRecording("001");

    {
        odb::transaction t(db->begin());
        db->persist(original_recording);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        QSharedPointer<Recording> loaded_recording(db->load<Recording>(original_recording.id()));
        t.commit();

        ASSERT_NE(loaded_recording, nullptr);
        expectRecordingsEqual(original_recording, *loaded_recording);
    }
}

// Test Case 2: Updating a Recording object
TEST_F(RecordingOdbTest, UpdateRecording)
{
    Recording recording_to_update = createSampleRecording("002");

    {
        odb::transaction t(db->begin());
        db->persist(recording_to_update);
        t.commit();
    }

    recording_to_update.setDuration(350.0f);
    recording_to_update.setObjects(25);
    recording_to_update.setDBFS(-30);
    recording_to_update.setEndTime(QDateTime::currentDateTimeUtc().addSecs(60));

    {
        odb::transaction t(db->begin());
        db->update(recording_to_update);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        QSharedPointer<Recording> loaded_recording(db->load<Recording>(recording_to_update.id()));
        t.commit();

        ASSERT_NE(loaded_recording, nullptr);
        expectRecordingsEqual(recording_to_update, *loaded_recording);
    }
}

// Test Case 3: Deleting a Recording object
TEST_F(RecordingOdbTest, DeleteRecording)
{
    Recording recording_to_delete = createSampleRecording("003");

    {
        odb::transaction t(db->begin());
        db->persist(recording_to_delete);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        ASSERT_NE(db->query_one<Recording>(odb::query<Recording>::id == recording_to_delete.id()), nullptr);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        db->erase(recording_to_delete);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        EXPECT_EQ(db->query_one<Recording>(odb::query<Recording>::id == recording_to_delete.id()), nullptr);
        t.commit();
    }
}

// Test Case 4: Querying Recording objects
TEST_F(RecordingOdbTest, QueryRecordings)
{
    Recording rec1 = createSampleRecording("A");
    rec1.setCamera("CamA");
    rec1.setMotion(100);
    rec1.setStartTime(QDateTime::fromString("2023-01-01T08:00:00", Qt::ISODate));

    Recording rec2 = createSampleRecording("B");
    rec2.setCamera("CamB");
    rec2.setMotion(500);
    rec2.setStartTime(QDateTime::fromString("2023-01-01T09:00:00", Qt::ISODate));

    Recording rec3 = createSampleRecording("C");
    rec3.setCamera("CamA");
    rec3.setMotion(200);
    rec3.setStartTime(QDateTime::fromString("2023-01-01T10:00:00", Qt::ISODate));

    {
        odb::transaction t(db->begin());
        db->persist(rec1);
        db->persist(rec2);
        db->persist(rec3);
        t.commit();
    }

    // Query by camera
    {
        odb::transaction t(db->begin());
        typedef odb::result<Recording> result_type;
        result_type r = db->query<Recording>(odb::query<Recording>::camera == "CamA");

        std::vector<Recording> found_recordings;
        for (Recording& rec : r) {
            found_recordings.push_back(rec);
        }
        t.commit();

        EXPECT_EQ(found_recordings.size(), 2);
        bool found1 = false, found3 = false;
        for (const auto& rec : found_recordings) {
            if (rec.id() == rec1.id()) found1 = true;
            if (rec.id() == rec3.id()) found3 = true;
        }
        EXPECT_TRUE(found1);
        EXPECT_TRUE(found3);
    }

    // Query by motion threshold
    {
        odb::transaction t(db->begin());
        typedef odb::result<Recording> result_type;
        result_type r = db->query<Recording>(odb::query<Recording>::motion > 150);

        std::vector<Recording> found_recordings;
        for (Recording& rec : r) {
            found_recordings.push_back(rec);
        }
        t.commit();

        EXPECT_EQ(found_recordings.size(), 2);
        bool found2 = false, found3 = false;
        for (const auto& rec : found_recordings) {
            if (rec.id() == rec2.id()) found2 = true;
            if (rec.id() == rec3.id()) found3 = true;
        }
        EXPECT_TRUE(found2);
        EXPECT_TRUE(found3);
    }
}
