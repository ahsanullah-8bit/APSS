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

#include <db/exports>

class ExportsOdbTest : public ::testing::Test
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

    Exports createSampleExport(const QString& idSuffix = "")
    {
        Exports exp;
        exp.setId("export_id_" + idSuffix);
        exp.setCamera("CameraX");
        exp.setName("My Exported Video");
        exp.setDate(QDateTime::currentDateTimeUtc().toTimeSpec(Qt::LocalTime));
        exp.setVideoPath("/exports/video_" + idSuffix + ".mp4");
        exp.setThumbPath("/exports/thumb_" + idSuffix + ".jpg");
        exp.setInProgress(false);
        return exp;
    }

    void expectExportsEqual(const Exports& expected, const Exports& actual)
    {
        EXPECT_EQ(actual.id(), expected.id());
        EXPECT_EQ(actual.camera(), expected.camera());
        EXPECT_EQ(actual.name(), expected.name());
        EXPECT_EQ(actual.date().toString(Qt::ISODateWithMs), expected.date().toString(Qt::ISODateWithMs));
        EXPECT_EQ(actual.videoPath(), expected.videoPath());
        EXPECT_EQ(actual.thumbPath(), expected.thumbPath());
        EXPECT_EQ(actual.inProgress(), expected.inProgress());
    }
};

TEST_F(ExportsOdbTest, PersistAndLoadExport)
{
    Exports original_export = createSampleExport("001");

    {
        odb::transaction t(db->begin());
        db->persist(original_export);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        QSharedPointer<Exports> loaded_export(db->load<Exports>(original_export.id()));
        t.commit();

        ASSERT_NE(loaded_export, nullptr);
        expectExportsEqual(original_export, *loaded_export);
    }
}

TEST_F(ExportsOdbTest, UpdateExport)
{
    Exports export_to_update = createSampleExport("002");

    {
        odb::transaction t(db->begin());
        db->persist(export_to_update);
        t.commit();
    }

    export_to_update.setName("Updated Export Name");
    export_to_update.setInProgress(true);
    export_to_update.setVideoPath("/new/path/video.mp4");

    {
        odb::transaction t(db->begin());
        db->update(export_to_update);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        QSharedPointer<Exports> loaded_export(db->load<Exports>(export_to_update.id()));
        t.commit();

        ASSERT_NE(loaded_export, nullptr);
        expectExportsEqual(export_to_update, *loaded_export);
    }
}

TEST_F(ExportsOdbTest, DeleteExport)
{
    Exports export_to_delete = createSampleExport("003");

    {
        odb::transaction t(db->begin());
        db->persist(export_to_delete);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        ASSERT_NE(db->query_one<Exports>(odb::query<Exports>::id == export_to_delete.id()), nullptr);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        db->erase(export_to_delete);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        EXPECT_EQ(db->query_one<Exports>(odb::query<Exports>::id == export_to_delete.id()), nullptr);
        t.commit();
    }
}

TEST_F(ExportsOdbTest, QueryExports)
{
    Exports exp1 = createSampleExport("A");
    exp1.setCamera("Cam1");
    exp1.setInProgress(false);

    Exports exp2 = createSampleExport("B");
    exp2.setCamera("Cam2");
    exp2.setInProgress(true);

    Exports exp3 = createSampleExport("C");
    exp3.setCamera("Cam1");
    exp3.setInProgress(true);

    {
        odb::transaction t(db->begin());
        db->persist(exp1);
        db->persist(exp2);
        db->persist(exp3);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        typedef odb::result<Exports> result_type;
        result_type r = db->query<Exports>(odb::query<Exports>::camera == "Cam1");

        std::vector<Exports> found_exports;
        for (Exports& e : r) {
            found_exports.push_back(e);
        }
        t.commit();

        EXPECT_EQ(found_exports.size(), 2);
        bool found1 = false, found3 = false;
        for (const auto& e : found_exports) {
            if (e.id() == exp1.id()) found1 = true;
            if (e.id() == exp3.id()) found3 = true;
        }
        EXPECT_TRUE(found1);
        EXPECT_TRUE(found3);
    }

    {
        odb::transaction t(db->begin());
        typedef odb::result<Exports> result_type;
        result_type r = db->query<Exports>(odb::query<Exports>::inProgress == true);

        std::vector<Exports> found_exports;
        for (Exports& e : r) {
            found_exports.push_back(e);
        }
        t.commit();

        EXPECT_EQ(found_exports.size(), 2);
        bool found2 = false, found3 = false;
        for (const auto& e : found_exports) {
            if (e.id() == exp2.id()) found2 = true;
            if (e.id() == exp3.id()) found3 = true;
        }
        EXPECT_TRUE(found2);
        EXPECT_TRUE(found3);
    }
}
