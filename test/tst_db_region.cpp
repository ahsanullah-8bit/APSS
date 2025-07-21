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
#include <db/region>

class TestOdbRegion : public ::testing::Test
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

    Region createSampleRegion(const QString& camera_id = "CameraDefault")
    {
        Region region;
        region.setCamera(camera_id);
        region.setGrid("1,1,1,1,0,0,0,0");
        region.setLastUpdate(QDateTime::currentDateTimeUtc());
        return region;
    }

    void expectRegionsEqual(const Region& expected, const Region& actual)
    {
        EXPECT_EQ(actual.camera(), expected.camera());
        EXPECT_EQ(actual.grid(), expected.grid());
        EXPECT_EQ(forceToUTC(actual.lastUpdate()).toString(Qt::ISODateWithMs), forceToUTC(expected.lastUpdate()).toString(Qt::ISODateWithMs));
    }
};

TEST_F(TestOdbRegion, PersistAndLoadRegion)
{
    Region original_region = createSampleRegion("FrontDoorCam");

    {
        odb::transaction t(db->begin());
        db->persist(original_region);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        QSharedPointer<Region> loaded_region(db->load<Region>(original_region.camera()));
        t.commit();

        ASSERT_NE(loaded_region, nullptr);
        expectRegionsEqual(original_region, *loaded_region);
    }
}

TEST_F(TestOdbRegion, UpdateRegion)
{
    Region region_to_update = createSampleRegion("BackyardCam");

    {
        odb::transaction t(db->begin());
        db->persist(region_to_update);
        t.commit();
    }

    region_to_update.setGrid("0,0,1,1,1,1,0,0");
    region_to_update.setLastUpdate(QDateTime::currentDateTimeUtc().addDays(1));

    {
        odb::transaction t(db->begin());
        db->update(region_to_update);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        QSharedPointer<Region> loaded_region(db->load<Region>(region_to_update.camera()));
        t.commit();

        ASSERT_NE(loaded_region, nullptr);
        expectRegionsEqual(region_to_update, *loaded_region);
    }
}

TEST_F(TestOdbRegion, DeleteRegion)
{
    Region region_to_delete = createSampleRegion("GarageCam");

    {
        odb::transaction t(db->begin());
        db->persist(region_to_delete);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        ASSERT_NE(db->query_one<Region>(odb::query<Region>::camera == region_to_delete.camera()), nullptr);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        db->erase(region_to_delete);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        EXPECT_EQ(db->query_one<Region>(odb::query<Region>::camera == region_to_delete.camera()), nullptr);
        t.commit();
    }
}

TEST_F(TestOdbRegion, QueryRegions)
{
    Region reg1 = createSampleRegion("CamA");
    reg1.setGrid("1,0,0,0");

    Region reg2 = createSampleRegion("CamB");
    reg2.setGrid("0,1,0,0");

    Region reg3 = createSampleRegion("CamC");
    reg3.setGrid("1,1,0,0");

    {
        odb::transaction t(db->begin());
        db->persist(reg1);
        db->persist(reg2);
        db->persist(reg3);
        t.commit();
    }

    {
        odb::transaction t(db->begin());
        typedef odb::result<Region> result_type;
        result_type r = db->query<Region>(odb::query<Region>::grid == "1,0,0,0");

        std::vector<Region> found_regions;
        for (Region& reg : r) {
            found_regions.push_back(reg);
        }
        t.commit();

        EXPECT_EQ(found_regions.size(), 1);
        ASSERT_FALSE(found_regions.empty());
        expectRegionsEqual(reg1, found_regions[0]);
    }
}
