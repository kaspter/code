#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <faiss/IndexFlat.h>
#include <faiss/IndexIVFFlat.h>


// apt install libsqlite3-dev libfaiss-dev libopenblas-dev libomp-dev
// clang++ faiss_object.cpp -lsqlite3 -lfaiss -fopenmp -lopenblas -lomp


// SQLite database file name
const char* db_file = "faces.db";

// Faiss parameters
const int dimension = 128; // Dimensionality of the feature vectors
const int nlist = 100;      // Number of clusters

// Function to retrieve the feature vector from the database based on name
int get_feature_by_name(sqlite3* db, const char* name, float* feature) {
    sqlite3_stmt* stmt;

    const char* select_feature_sql = "SELECT FeatureVector FROM Faces WHERE Name = ?;";

    int rc = sqlite3_prepare_v2(db, select_feature_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare select statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const void* feature_blob = sqlite3_column_blob(stmt, 0);
        int feature_size = sqlite3_column_bytes(stmt, 0);
        if (feature_size != dimension * sizeof(float)) {
            fprintf(stderr, "Invalid feature size retrieved from the database.\n");
            sqlite3_finalize(stmt);
            return -1;
        }
        memcpy(feature, feature_blob, feature_size);
    } else {
        fprintf(stderr, "Failed to execute select statement: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);
    return 0;
}


int build_faiss_index_from_sqlite(sqlite3* db, faiss::IndexFlatL2& index)
{
    // Populate Faiss index with feature vectors from the database
    sqlite3_stmt* stmt;
    const char* select_all_features_sql = "SELECT FeatureVector FROM Faces;";
    int rc = sqlite3_prepare_v2(db, select_all_features_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare select statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const void* feature_blob = sqlite3_column_blob(stmt, 0);
        int feature_size = sqlite3_column_bytes(stmt, 0);
        if (feature_size != dimension * sizeof(float)) {
            fprintf(stderr, "Invalid feature size retrieved from the database.\n");
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return 1;
        }
        faiss::Index::idx_t id = index.ntotal; // Assign sequential IDs to the vectors
        index.add(1, (float*)feature_blob);
    }

    sqlite3_finalize(stmt);

    return 0;
}


int main() {
    sqlite3* db;
    int rc = sqlite3_open(db_file, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    // Create Faiss index
    faiss::IndexFlatL2 index(dimension); // L2 distance is used

    // 从SQLite构建Faiss索引
    build_faiss_index_from_sqlite(db, index);


    // Perform face search
    const char* query_name = "阿斯顿";
    float query_feature[dimension];

    if (get_feature_by_name(db, query_name, query_feature) == 0) {


        for (int i = 0; i < dimension; i++) {
            printf("%.2f ", query_feature[i]);
        }
        printf("\n");


        // Search for similar faces
        const int k = 2; // Number of nearest neighbors to retrieve
        faiss::Index::idx_t* I = new faiss::Index::idx_t[k];
        float* D = new float[k];
        index.search(1, query_feature, k, D, I);

        // Print results
        printf("Similar faces to '%s':\n", query_name);
        for (int i = 0; i < k; i++) {
            printf("Rank %d: ID %ld, Distance %.2f\n", i+1, I[i], D[i]);
        }

        delete[] I;
        delete[] D;
    } else {
        fprintf(stderr, "Failed to retrieve query feature vector.\n");
    }

    // Close SQLite database
    sqlite3_close(db);

    return 0;
}










