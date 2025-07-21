#include "gtest/gtest.h"

#include <odb/core.hxx>
#include <odb/database.hxx>
#include <odb/transaction.hxx>
#include <odb/session.hxx>
#include <odb/schema-catalog.hxx>

#include <odb/sqlite/database.hxx>

#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>

#include "db/db.h"
#include <db/timeline>

class TestOdbTimeline : public ::testing::Test
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
    {}

    Timeline createSampleTimeline(long id)
    {
        Timeline tl;
        tl.setId(id);
        tl.setTimestamp(QDateTime::currentDateTimeUtc());
        tl.setCamera("CameraZ");
        tl.setSource("Event");
        tl.setSourceId("event_xyz");
        tl.setClassType("Person");
        tl.setData("{\"event_data\":\"some_json\"}");
        return tl;
    }

    void expectTimelinesEqual(const Timeline& expected, const Timeline& actual)
    {
        EXPECT_EQ(actual.id(), expected.id());
        EXPECT_EQ(forceToUTC(actual.timestamp()).toString(Qt::ISODateWithMs), forceToUTC(expected.timestamp()).toString(Qt::ISODateWithMs));
        EXPECT_EQ(actual.camera(), expected.camera());
        EXPECT_EQ(actual.source(), expected.source());
        EXPECT_EQ(actual.sourceId(), expected.sourceId());
        EXPECT_EQ(actual.classType(), expected.classType());
        EXPECT_EQ(actual.data(), expected.data());
    }
};

TEST_F(TestOdbTimeline, PersistAndLoadTimeline)
{
    Timeline original_timeline = createSampleTimeline(1001);

    {
        odb::transaction t(db->begin());
        db->persist(original_timeline);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        QSharedPointer<Timeline> loaded_timeline(db->load<Timeline>(original_timeline.id()));
        t.commit();

        ASSERT_NE(loaded_timeline, nullptr);
        expectTimelinesEqual(original_timeline, *loaded_timeline);
    }
}

TEST_F(TestOdbTimeline, UpdateTimeline)
{
    Timeline timeline_to_update = createSampleTimeline(1002);

    {
        odb::transaction t(db->begin());
        db->persist(timeline_to_update);
        t.commit();
    }

    timeline_to_update.setClassType("Vehicle");
    timeline_to_update.setData("{\"vehicle_type\":\"car\"}");
    timeline_to_update.setTimestamp(QDateTime::currentDateTimeUtc().addSecs(30));

    {
        odb::transaction t(db->begin());
        db->update(timeline_to_update);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        QSharedPointer<Timeline> loaded_timeline(db->load<Timeline>(timeline_to_update.id()));
        t.commit();

        ASSERT_NE(loaded_timeline, nullptr);
        expectTimelinesEqual(timeline_to_update, *loaded_timeline);
    }
}

TEST_F(TestOdbTimeline, DeleteTimeline)
{
    Timeline timeline_to_delete = createSampleTimeline(1003);

    {
        odb::transaction t(db->begin());
        db->persist(timeline_to_delete);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        ASSERT_NE(db->query_one<Timeline>(odb::query<Timeline>::id == timeline_to_delete.id()), nullptr);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        db->erase(timeline_to_delete);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        EXPECT_EQ(db->query_one<Timeline>(odb::query<Timeline>::id == timeline_to_delete.id()), nullptr);
        t.commit();
    }
}

TEST_F(TestOdbTimeline, QueryTimelines)
{
    Timeline tl1 = createSampleTimeline(2001);
    tl1.setCamera("CamA");
    tl1.setClassType("Person");
    tl1.setTimestamp(QDateTime::fromString("2023-01-01T12:00:00", Qt::ISODate));

    Timeline tl2 = createSampleTimeline(2002);
    tl2.setCamera("CamB");
    tl2.setClassType("Vehicle");
    tl2.setTimestamp(QDateTime::fromString("2023-01-01T12:30:00", Qt::ISODate));

    Timeline tl3 = createSampleTimeline(2003);
    tl3.setCamera("CamA");
    tl3.setClassType("Animal");
    tl3.setTimestamp(QDateTime::fromString("2023-01-01T13:00:00", Qt::ISODate));

    {
        odb::transaction t(db->begin());
        db->persist(tl1);
        db->persist(tl2);
        db->persist(tl3);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        typedef odb::result<Timeline> result_type;
        result_type r = db->query<Timeline>(odb::query<Timeline>::camera == "CamA");

        std::vector<Timeline> found_timelines;
        for (Timeline& tl : r) {
            found_timelines.push_back(tl);
        }
        t.commit();

        EXPECT_EQ(found_timelines.size(), 2);
        bool found1 = false, found3 = false;
        for (const auto& tl : found_timelines) {
            if (tl.id() == tl1.id()) found1 = true;
            if (tl.id() == tl3.id()) found3 = true;
        }
        EXPECT_TRUE(found1);
        EXPECT_TRUE(found3);
    }

    {
        odb::transaction t(db->begin());
        typedef odb::result<Timeline> result_type;
        QDateTime start_range = QDateTime::fromString("2023-01-01T12:15:00", Qt::ISODate);
        QDateTime end_range = QDateTime::fromString("2023-01-01T12:45:00", Qt::ISODate);

        result_type r = db->query<Timeline>(
            odb::query<Timeline>::classType == "Vehicle" &&
            odb::query<Timeline>::timestamp >= start_range &&
            odb::query<Timeline>::timestamp <= end_range);

        std::vector<Timeline> found_timelines;
        for (Timeline& tl : r) {
            found_timelines.push_back(tl);
        }
        t.commit();

        EXPECT_EQ(found_timelines.size(), 1);
        ASSERT_FALSE(found_timelines.empty());
        expectTimelinesEqual(tl2, found_timelines[0]);
    }
}
