#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>


// gcc sqlite_object.c -lsqlite3 -o sqldb

// SQLite数据库文件名
const char* db_file = "faces.db";

// 创建表格的SQL语句
const char* create_table_sql = "CREATE TABLE IF NOT EXISTS Faces (\
                                ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,\
                                Name TEXT,\
                                Age INTEGER,\
                                Gender TEXT,\
                                Hairstyle TEXT,\
                                FeatureVersion INTEGER,\
                                FeatureVector BLOB);";

struct object
{
    char *name;
    int age;
    char *gender;
    char *hairstyle;
    int version;
    void* feature;
    int feature_size;
};


// 添加人脸到数据库
void sqlite_add_face(sqlite3* db, const char* name, int age,
                     const char* gender, const char* hairstyle,
                     int version, const void* feature, int feature_size)
{
    sqlite3_stmt* stmt;

    // 插入人脸数据的SQL语句
    const char* insert_face_sql = "INSERT INTO Faces (Name, Age, Gender, Hairstyle, FeatureVersion, FeatureVector)\
                                    VALUES (?, ?, ?, ?, ?, ?);";


    int rc = sqlite3_prepare_v2(db, insert_face_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare insert statement: %s\n", sqlite3_errmsg(db));
        return;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, age);
    sqlite3_bind_text(stmt, 3, gender, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, hairstyle, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, version);
    sqlite3_bind_blob(stmt, 6, feature, feature_size, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to execute insert statement: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
}


// Function to add a face to the database
void add_face(sqlite3* db, struct object *face)
{
    sqlite_add_face(db, face->name, face->age, face->gender, face->hairstyle, face->version, face->feature, face->feature_size);
}


// Function to retrieve the feature vector from the database based on name
// Select feature data SQL statement (based on name)

int get_feature_by_name(sqlite3* db, const char* name, void** feature, int* feature_size, int* version) {
    sqlite3_stmt* stmt;

    const char* select_feature_sql = "SELECT FeatureVector, FeatureVersion FROM Faces WHERE Name = ?;";

    int rc = sqlite3_prepare_v2(db, select_feature_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare select statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        *feature = malloc(sqlite3_column_bytes(stmt, 0));
        if (*feature == NULL) {
            fprintf(stderr, "Failed to allocate memory\n");
            return -1;
        }
        *feature_size = sqlite3_column_bytes(stmt, 0);
        *version = sqlite3_column_int(stmt, 1);
        memcpy(*feature, sqlite3_column_blob(stmt, 0), *feature_size);
    } else {
        fprintf(stderr, "Failed to execute select statement: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);

    return 0;
}

int sqlite_face_count(sqlite3* db)
{
    sqlite3_stmt* stmt;
    int count = 0;
    const char* count_faces_sql = "SELECT COUNT(*) FROM Faces;";
    int rc = sqlite3_prepare_v2(db, count_faces_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare count statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    } else {
        fprintf(stderr, "Failed to execute count statement: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    return count;
}


// 从数据库中删除人脸
void delete_face_by_id(sqlite3* db, int id) {
    sqlite3_stmt* stmt;
    // 删除人脸数据的SQL语句
    const char* delete_face_sql = "DELETE FROM Faces WHERE ID = ?;";

    int rc = sqlite3_prepare_v2(db, delete_face_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare delete statement: %s\n", sqlite3_errmsg(db));
        return;
    }
    sqlite3_bind_int(stmt, 1, id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to execute delete statement: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
}


// 从数据库中删除人脸（根据Name）
void delete_face_by_name(sqlite3* db, const char* name) {
    sqlite3_stmt* stmt;

    // 删除人脸数据的SQL语句（根据Name）
    const char* delete_face_by_name_sql = "DELETE FROM Faces WHERE Name = ?;";

    int rc = sqlite3_prepare_v2(db, delete_face_by_name_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare delete statement: %s\n", sqlite3_errmsg(db));
        return;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to execute delete statement: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
}


void print_feature(void* feature, int feature_size) {
    float* float_feature = (float*)feature;
    int num_floats = feature_size / sizeof(float);
    printf("Feature vector: \n");
    for (int i = 0; i < num_floats; i++) {
        printf("%.2f ", float_feature[i]);
    }
    printf("\n");
}


float* create_vectors(size_t const count, size_t const dimensions) {
    float* data = (float*)malloc(count * dimensions * sizeof(float));
    if (!data)
        return NULL;
    for (size_t index = 0; index < count * dimensions; ++index)
        data[index] = (float)rand() / (float)RAND_MAX;
    return data;
}


int main() {
    sqlite3* db;
    int rc = sqlite3_open(db_file, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    // 创建表格
    rc = sqlite3_exec(db, create_table_sql, NULL, 0, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to create table: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    // 示例人脸数据
    //float feature[] = {0.1, 0.2, 0.3}; // 示例特征向量

    float feature[128]; // 数组长度为 128
    for (int i = 0; i < 128; ++i) {
         // 填充示例特征向量
        //feature[i] = 0.1 * (i + 1);
        feature[i] = (float)rand() / (float)RAND_MAX;
    }

    int version = 1; // 版本号
    int feature_size = sizeof(feature);
    const char* name = "John";
    int age = 30;
    const char* gender = "Male";
    const char* hairstyle = "Short";

    fprintf(stderr, "feature_size: %d\n", feature_size);
    // 添加人脸到数据库
    sqlite_add_face(db, name, age, gender, hairstyle, version, feature, feature_size);


    for (int i = 0; i < 200; ++i) {
        // add again
        for (int i = 0; i < 128; ++i) {
             // 填充示例特征向量
            //feature[i] = 0.1 * (i + 1);
            feature[i] = (float)rand() / (float)RAND_MAX;
        }
        sqlite_add_face(db, "阿斯顿", age, gender, hairstyle, version, feature, feature_size);
    }

    {
        struct object face;

        for (int i = 0; i < 128; ++i) {
             // 填充示例特征向量
            //feature[i] = 0.1 * (i + 1);
            feature[i] = (float)rand() / (float)RAND_MAX;
        }


        face.name = "斯蒂芬";
        face.age = 30;
        face.gender = "Male";
        face.hairstyle = "Short";
        face.version = 1;
        face.feature = feature;
        face.feature_size = sizeof(feature);

        // Add face to the database
        add_face(db, &face);
    }

    // 删除示例人脸
    //delete_face_by_id(db, 1);


    {
        // Example: Get feature by name
        const char* name = "John";
        void* feature;
        int feature_size;
        int version;
        if (get_feature_by_name(db, name, &feature, &feature_size, &version) == 0) {
            printf("Feature vector retrieved successfully size %d\n", feature_size);
            // Now, feature vector is stored in 'feature' pointer, and its size is 'feature_size'
            // Don't forget to free 'feature' when you're done with it
            print_feature(feature, feature_size);
            free(feature);
        } else {
            fprintf(stderr, "Failed to retrieve feature vector.\n");
        }
    }


    fprintf(stderr, "max user id: %d\n", sqlite_face_count(db));

    sqlite3_close(db);
    return 0;
}
