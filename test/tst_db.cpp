#include <QObject>
#include <QTest>

#include <odbmodels/event>
#include <odbmodels/export>
#include <odbmodels/recording>
#include <odbmodels/region>
#include <odbmodels/timeline>

class TestDatabase : public QObject {
    Q_OBJECT

public:
    TestDatabase() = default;

private slots:
    void initTestCase();
    void modelEvent();
    void modelEvent_data();
    void modelExport();
    void modelExport_data();
    void modelRecordings();
    void modelRecordings_data();
    void modelRegions();
    void modelRegions_data();
    void modelTimeline();
    void modelTimeline_data();

private:
    std::string m_pathPrefix = "test/db";
};

void TestDatabase::initTestCase()
{
    // Ensure directories
    if (!std::filesystem::exists(m_pathPrefix)) {
        std::filesystem::create_directories(m_pathPrefix);
    }
    QVERIFY(std::filesystem::exists(m_pathPrefix));

    // // Init database
    // qx::QxSqlDatabase *singleton = qx::QxSqlDatabase::getSingleton();
    // singleton->setDriverName("QSQLITE");
    // singleton->setDatabaseName(QString::fromStdString(m_pathPrefix + "/test_qxorm.db"));
    // singleton->setHostName("localhost");
    // singleton->setUserName("root");
    // singleton->setPassword("");

    // // Create tables
    // QSqlDatabase db = singleton->getDatabaseCloned();
    // QStringList tables = db.tables();
    // qDebug() << tables;
    // if (!tables.contains("Events"))
    //     QCOMPARE(qx::dao::create_table<Event>().type(), QSqlError::NoError);

    // if (!tables.contains("Export"))
    //     QCOMPARE(qx::dao::create_table<Export>().type(), QSqlError::NoError);

    // if (!tables.contains("Recordings"))
    //     QCOMPARE(qx::dao::create_table<Recordings>().type(), QSqlError::NoError);

    // if (!tables.contains("Regions"))
    //     QCOMPARE(qx::dao::create_table<Regions>().type(), QSqlError::NoError);

    // if (!tables.contains("Timeline"))
    //     QCOMPARE(qx::dao::create_table<Timeline>().type(), QSqlError::NoError);
}

void TestDatabase::modelEvent()
{
    // Create a test Event instance
    Event event;
    event.setid("event_001");
    event.setlabel("Intrusion Alert");
    event.setsubLabel("Unauthorized Entry");
    event.setcamera("Front Gate");
    event.setstartTime(QDateTime::currentDateTime());
    event.setendTime(QDateTime::currentDateTime().addSecs(60));
    event.settopScore(0.95);
    event.setscore(0.85);
    event.setfalsePositive(false);
    event.setzones("Zone A");
    event.setthumbnail("images/event_001.jpg");
    event.sethasClip(true);
    event.sethasSnapshot(true);
    event.setregion("Region 1");
    event.setbox("100,200,300,400");
    event.setarea(5000);
    event.setretainIndefinitely(false);
    event.setratio(1.5);
    event.setplusId("plus_123");
    event.setmodelHash("hash_abc");
    event.setdetectorType("YOLOX");
    event.setmodelType("SSD");
    event.setdata("Additional metadata");

    // Serialize to database using QxOrm
    QSqlError result = qx::dao::insert(event);
    QVERIFY(result.type() == QSqlError::NoError);

    // Fetch the event back from the database
    Event fetchedEvent;
    fetchedEvent.setid("event_001");
    qx::dao::fetch_by_id(fetchedEvent);

    // Compare serialized and deserialized data
    QCOMPARE(event.getid(), fetchedEvent.getid());
    QCOMPARE(event.getlabel(), fetchedEvent.getlabel());
    QCOMPARE(event.getcamera(), fetchedEvent.getcamera());
    QCOMPARE(event.getstartTime(), fetchedEvent.getstartTime());
    QCOMPARE(event.getendTime(), fetchedEvent.getendTime());
    QCOMPARE(event.getscore(), fetchedEvent.getscore());
    QCOMPARE(event.getmodelType(), fetchedEvent.getmodelType());
}

void TestDatabase::modelEvent_data()
{

}

void TestDatabase::modelExport()
{

}

void TestDatabase::modelExport_data()
{

}

void TestDatabase::modelRecordings()
{

}

void TestDatabase::modelRecordings_data()
{

}

void TestDatabase::modelRegions()
{

}

void TestDatabase::modelRegions_data()
{

}

void TestDatabase::modelTimeline()
{

}

void TestDatabase::modelTimeline_data()
{

}



QTEST_MAIN(TestDatabase);
#include "tst_db.moc"
