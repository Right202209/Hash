#include <QCoreApplication>
#include <QtTest>

#include "test_atomicfile.h"
#include "test_batch.h"
#include "test_compare.h"
#include "test_format.h"
#include "test_hasher.h"
#include "test_pathkey.h"
#include "test_paths.h"
#include "test_progress.h"
#include "test_registry.h"

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    int status = 0;
    {
        TestRegistry test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        TestHasher test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        TestBatch test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        TestAtomicFile test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        TestCompare test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        TestFormat test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        TestPaths test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        TestPathKey test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        TestProgress test;
        status |= QTest::qExec(&test, argc, argv);
    }
    return status == 0 ? 0 : 1;
}
