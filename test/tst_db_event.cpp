#include <filesystem>
#include <gtest/gtest.h>

#include <odb/core.hxx>
#include <odb/database.hxx>
#include <odb/transaction.hxx>
#include <odb/session.hxx>
#include <odb/schema-catalog.hxx> // For create_schema

#include <odb/sqlite/database.hxx>

#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>

#include "db/db.h"
#include "sqlite/event-odb.hxx"

// Helper for Google Test to print QStrings in readable format
inline std::ostream& operator<<(std::ostream& os, const QString& str)
{
    return os << str.toStdString();
}

class EventOdbTest : public ::testing::Test
{
protected:
    std::string m_pathPrefix = "test/db";
    std::unique_ptr<odb::database> db;

    void SetUp() override
    {
        // Make sure the paths exist
        std::filesystem::create_directories(m_pathPrefix);

        // Create a .db SQLite database.
        db = std::make_unique<odb::sqlite::database>(m_pathPrefix + "/test_db_event.db", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);

        // Create the database schema (tables, indices, etc.) for all registered ODB objects.
        // This must be done within a transaction.
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

    Event createSampleEvent(const QString& idSuffix = "")
    {
        Event event;
        event.setId("event_id_" + idSuffix);
        event.setLabel("Person Detected");
        event.setSubLabel("Face Recognized");
        event.setCamera("Camera1");
        event.setStartTime(QDateTime::fromString("2025-07-06 10:00:00.000Z", Qt::ISODateWithMs));
        event.setEndTime(QDateTime::fromString(  "2025-07-06 10:01:00.000Z", Qt::ISODateWithMs));

        if (idSuffix == "M1") {
            event.setStartTime(QDateTime::fromString("2025-07-06 10:00:00.000Z", Qt::ISODateWithMs));
            event.setEndTime(QDateTime::fromString(  "2025-07-06 10:01:00.000Z", Qt::ISODateWithMs));
        } else if (idSuffix == "M2") {
            event.setStartTime(QDateTime::fromString("2025-07-06 10:05:00.000Z", Qt::ISODateWithMs));
            event.setEndTime(QDateTime::fromString(  "2025-07-06 10:06:00.000Z", Qt::ISODateWithMs));
        } else if (idSuffix == "M3") {
            event.setStartTime(QDateTime::fromString("2025-07-06 10:10:00.000Z", Qt::ISODateWithMs));
            event.setEndTime(QDateTime::fromString(  "2025-07-06 10:11:00.000Z", Qt::ISODateWithMs));
        }

        event.setTopScore(0.98f);
        event.setScore(0.85f);
        event.setFalsePositive(false);
        event.setZones("ZoneA,ZoneB");
        event.setThumbnail("/path/to/thumbnail.jpg");
        event.setHasClip(true);
        event.setHasSnapshot(true);
        event.setRegion("Region1");
        event.setBox("10,20,30,40");
        event.setArea(1200);
        event.setRetainIndefinitely(false);
        event.setRatio(1.77f);
        event.setPlusId("plus_id_xyz");
        event.setModelHash("model_hash_abc");
        event.setDetectorType("YOLO");
        event.setModelType("v5s");
        event.setData("{\"confidence\":0.9,\"objects\":[\"person\"]}");
        return event;
    }

    void expectEventsEqual(const Event& expected, const Event& actual)
    {
        EXPECT_EQ(actual.id(), expected.id());
        EXPECT_EQ(actual.label(), expected.label());
        EXPECT_EQ(actual.subLabel(), expected.subLabel());
        EXPECT_EQ(actual.camera(), expected.camera());
        EXPECT_EQ(forceToUTC(actual.startTime()).toString(Qt::ISODateWithMs), forceToUTC(expected.startTime()).toString(Qt::ISODateWithMs));
        EXPECT_EQ(forceToUTC(actual.endTime()).toString(Qt::ISODateWithMs), forceToUTC(expected.endTime()).toString(Qt::ISODateWithMs));
        EXPECT_FLOAT_EQ(actual.topScore(), expected.topScore());
        EXPECT_FLOAT_EQ(actual.score(), expected.score());
        EXPECT_EQ(actual.falsePositive(), expected.falsePositive());
        EXPECT_EQ(actual.zones(), expected.zones());
        EXPECT_EQ(actual.thumbnail(), expected.thumbnail());
        EXPECT_EQ(actual.hasClip(), expected.hasClip());
        EXPECT_EQ(actual.hasSnapshot(), expected.hasSnapshot());
        EXPECT_EQ(actual.region(), expected.region());
        EXPECT_EQ(actual.box(), expected.box());
        EXPECT_EQ(actual.area(), expected.area());
        EXPECT_EQ(actual.retainIndefinitely(), expected.retainIndefinitely());
        EXPECT_FLOAT_EQ(actual.ratio(), expected.ratio());
        EXPECT_EQ(actual.plusId(), expected.plusId());
        EXPECT_EQ(actual.modelHash(), expected.modelHash());
        EXPECT_EQ(actual.detectorType(), expected.detectorType());
        EXPECT_EQ(actual.modelType(), expected.modelType());
        EXPECT_EQ(actual.data(), expected.data());
    }
};

TEST_F(EventOdbTest, PersistAndLoadEvent)
{
    Event original_event = createSampleEvent("001");

    // Persist the object
    {
        odb::transaction t(db->begin());
        db->persist(original_event);
        t.commit();
    }

    // Load the object by its ID
    {
        odb::transaction t(db->begin());
        QSharedPointer<Event> loaded_event(db->load<Event>(original_event.id()));
        t.commit();

        ASSERT_NE(loaded_event, nullptr) << "Failed to load Event object.";
        expectEventsEqual(original_event, *loaded_event);
    }
}

TEST_F(EventOdbTest, UpdateEvent)
{
    Event event_to_update = createSampleEvent("002");

    {
        odb::transaction t(db->begin());
        db->persist(event_to_update);
        t.commit();
    }

    event_to_update.setLabel("Updated Label");
    event_to_update.setScore(0.99f);
    event_to_update.setFalsePositive(true);
    event_to_update.setEndTime(QDateTime::currentDateTimeUtc().addSecs(120));

    {
        odb::transaction t(db->begin());
        db->update(event_to_update);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        QSharedPointer<Event> loaded_event(db->load<Event>(event_to_update.id()));
        t.commit();

        ASSERT_NE(loaded_event, nullptr) << "Failed to load updated Event object.";
        expectEventsEqual(event_to_update, *loaded_event);
    }
}

TEST_F(EventOdbTest, DeleteEvent)
{
    Event event_to_delete = createSampleEvent("003");

    {
        odb::transaction t(db->begin());
        db->persist(event_to_delete);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        QSharedPointer<Event> found_event = db->query_one<Event>(odb::query<Event>::id == event_to_delete.id());
        ASSERT_NE(found_event, nullptr)
            << "Event should exist before deletion.";
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        db->erase(event_to_delete);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        QSharedPointer<Event> found_event = db->query_one<Event>(odb::query<Event>::id == event_to_delete.id());
        EXPECT_EQ(found_event, nullptr);
        t.commit();
    }
}

TEST_F(EventOdbTest, QueryEvents)
{
    Event event1 = createSampleEvent("A");
    event1.setCamera("CameraA");
    event1.setLabel("Motion");
    event1.setStartTime(QDateTime::fromString("2023-01-01T10:00:00", Qt::ISODate));

    Event event2 = createSampleEvent("B");
    event2.setCamera("CameraB");
    event2.setLabel("Person");
    event2.setStartTime(QDateTime::fromString("2023-01-01T10:30:00", Qt::ISODate));

    Event event3 = createSampleEvent("C");
    event3.setCamera("CameraA"); // Same camera as event1
    event3.setLabel("Motion");   // Same label as event1
    event3.setStartTime(QDateTime::fromString("2023-01-01T11:00:00", Qt::ISODate));

    {
        odb::transaction t(db->begin());
        db->persist(event1);
        db->persist(event2);
        db->persist(event3);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        typedef odb::result<Event> result_type;
        result_type r = db->query<Event>(odb::query<Event>::camera == "CameraA");

        std::vector<Event> found_events;
        for (Event& e : r) {
            found_events.push_back(e);
        }
        t.commit();

        EXPECT_EQ(found_events.size(), 2);
        bool found1 = false, found3 = false;
        for (const auto& e : found_events) {
            if (e.id() == event1.id()) found1 = true;
            if (e.id() == event3.id()) found3 = true;
        }
        EXPECT_TRUE(found1);
        EXPECT_TRUE(found3);
    }

    {
        odb::transaction t(db->begin());
        typedef odb::result<Event> result_type;
        result_type r = db->query<Event>(
            odb::query<Event>::label == "Motion" &&
            odb::query<Event>::camera == "CameraA");

        std::vector<Event> found_events;
        for (Event& e : r) {
            found_events.push_back(e);
        }
        t.commit();

        EXPECT_EQ(found_events.size(), 2);
        bool found1 = false, found3 = false;
        for (const auto& e : found_events) {
            if (e.id() == event1.id()) found1 = true;
            if (e.id() == event3.id()) found3 = true;
        }
        EXPECT_TRUE(found1);
        EXPECT_TRUE(found3);
    }

    {
        odb::transaction t(db->begin());
        typedef odb::result<Event> result_type;
        QDateTime start_range = QDateTime::fromString("2023-01-01T10:15:00", Qt::ISODate);
        QDateTime end_range = QDateTime::fromString("2023-01-01T10:45:00", Qt::ISODate);

        result_type r = db->query<Event>(
            odb::query<Event>::startTime >= start_range &&
            odb::query<Event>::startTime <= end_range);

        std::vector<Event> found_events;
        for (Event& e : r) {
            found_events.push_back(e);
        }
        t.commit();

        EXPECT_EQ(found_events.size(), 1);
        ASSERT_FALSE(found_events.empty());
        EXPECT_EQ(found_events[0].id(), event2.id());
    }
}

TEST_F(EventOdbTest, DefaultConstructorAndEmptyValues)
{
    Event default_event;
    default_event.setId("default_test");

    {
        odb::transaction t(db->begin());
        db->persist(default_event);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        QSharedPointer<Event> loaded_event(db->load<Event>(default_event.id()));
        t.commit();

        ASSERT_NE(loaded_event, nullptr);
        EXPECT_EQ(loaded_event->id(), "default_test");
        EXPECT_TRUE(loaded_event->label().isEmpty());
        EXPECT_TRUE(loaded_event->startTime().isNull());
        EXPECT_FLOAT_EQ(loaded_event->topScore(), 0.0f);
        EXPECT_EQ(loaded_event->falsePositive(), false);
    }
}

TEST_F(EventOdbTest, PersistMultipleEvents)
{
    std::vector<Event> original_events;
    original_events.push_back(createSampleEvent("M1"));
    original_events.push_back(createSampleEvent("M2"));
    original_events.push_back(createSampleEvent("M3"));

    {
        odb::transaction t(db->begin());
        for (const auto& event : original_events) {
            db->persist(event);
        }
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        typedef odb::result<Event> result_type;
        result_type r = db->query<Event>(); // Load all events

        std::vector<Event> loaded_events;
        for (Event& e : r) {
            loaded_events.push_back(e);
        }
        t.commit();

        EXPECT_EQ(loaded_events.size(), original_events.size());

        for (const auto& original : original_events) {
            bool found = false;
            for (const auto& loaded : loaded_events) {
                if (loaded.id() == original.id()) {
                    expectEventsEqual(original, loaded);
                    found = true;
                    break;
                }
            }
            EXPECT_TRUE(found);
        }
    }
}
