# JinWo VecDB R Demo

# 加载必要的包
library(Rcpp)

# 定义搜索结果结构体
cppFunction('  
  struct SearchResult {
    uint64_t id;
    float distance;
  };
')

# 定义C API接口
cppFunction('  
  #include <string>
  #include <vector>
  
  // 声明C函数
  extern "C" {
    int jw_vecdb_open(void **db, const char *path, int create);
    int jw_vecdb_close(void *db);
    const char *jw_version();
    int jw_collection_create(void **collection, void *db, const char *name, int dimension);
    int jw_collection_close(void *collection);
    int jw_collection_list(void *db, char ***names, size_t *count);
    int jw_collection_insert(void *collection, const float *vector, uint64_t *id);
    int jw_collection_search(void *collection, const float *query, size_t k, struct SearchResult **results, size_t *count);
    void free(void *ptr);
  }
  
  // 加载库
  std::string loadLibrary() {
    std::string libraryPath;
    #ifdef __linux__
    libraryPath = "../../build/libjw_vecdb.so";
    #elif defined(_WIN32)
    libraryPath = "../../build/jw_vecdb.dll";
    #elif defined(__APPLE__)
    libraryPath = "../../build/libjw_vecdb.dylib";
    #else
    return "Unsupported platform";
    #endif
    return libraryPath;
  }
  
  // 获取版本
  std::string getVersion() {
    const char *version = jw_version();
    if (version) {
      return std::string(version);
    }
    return "Failed to get version";
  }
  
  // 打开数据库
  SEXP openDatabase(const char *path, int create) {
    void *db = NULL;
    int result = jw_vecdb_open(&db, path, create);
    if (result != 0) {
      return Rcpp::wrap(-1);
    }
    return Rcpp::XPtr<void>(db, true);
  }
  
  // 关闭数据库
  int closeDatabase(SEXP dbPtr) {
    Rcpp::XPtr<void> db(dbPtr);
    return jw_vecdb_close(db.get());
  }
  
  // 创建集合
  SEXP createCollection(SEXP dbPtr, const char *name, int dimension) {
    Rcpp::XPtr<void> db(dbPtr);
    void *collection = NULL;
    int result = jw_collection_create(&collection, db.get(), name, dimension);
    if (result != 0) {
      return Rcpp::wrap(-1);
    }
    return Rcpp::XPtr<void>(collection, true);
  }
  
  // 插入向量
  uint64_t insertVector(SEXP collectionPtr, Rcpp::NumericVector vector) {
    Rcpp::XPtr<void> collection(collectionPtr);
    uint64_t id = 0;
    std::vector<float> vec(vector.begin(), vector.end());
    int result = jw_collection_insert(collection.get(), vec.data(), &id);
    if (result != 0) {
      return 0;
    }
    return id;
  }
  
  // 搜索向量
  Rcpp::List searchVector(SEXP collectionPtr, Rcpp::NumericVector query, size_t k) {
    Rcpp::XPtr<void> collection(collectionPtr);
    struct SearchResult **results = NULL;
    size_t count = 0;
    std::vector<float> q(query.begin(), query.end());
    int result = jw_collection_search(collection.get(), q.data(), k, results, &count);
    if (result != 0) {
      return Rcpp::List::create();
    }
    
    Rcpp::NumericVector ids(count);
    Rcpp::NumericVector distances(count);
    for (size_t i = 0; i < count; i++) {
      ids[i] = results[i]->id;
      distances[i] = results[i]->distance;
      free(results[i]);
    }
    free(results);
    
    return Rcpp::List::create(
      Rcpp::Named("ids") = ids,
      Rcpp::Named("distances") = distances
    );
  }
  
  // 列出集合
  Rcpp::CharacterVector listCollections(SEXP dbPtr) {
    Rcpp::XPtr<void> db(dbPtr);
    char **names = NULL;
    size_t count = 0;
    int result = jw_collection_list(db.get(), &names, &count);
    if (result != 0) {
      return Rcpp::CharacterVector::create();
    }
    
    Rcpp::CharacterVector collectionNames(count);
    for (size_t i = 0; i < count; i++) {
      collectionNames[i] = std::string(names[i]);
      free(names[i]);
    }
    free(names);
    
    return collectionNames;
  }
', verbose = FALSE)

# 主函数
main <- function() {
  cat("JinWo VecDB R Demo\n")
  cat("==================\n")
  
  # 1. 加载库
  library_path <- loadLibrary()
  cat(paste("Using library:", library_path, "\n"))
  
  # 2. 测试版本信息
  version <- getVersion()
  cat(paste("Version:", version, "\n"))
  
  # 3. 打开数据库
  db_path <- "test_db_r"
  create <- 1  # 创建新数据库
  db <- openDatabase(db_path, create)
  if (db == -1) {
    cat("Failed to open database\n")
    return
  }
  cat("Database opened successfully\n")
  
  # 4. 创建集合
  collection_name <- "test_collection"
  dimension <- 128
  collection <- createCollection(db, collection_name, dimension)
  if (collection == -1) {
    cat("Failed to create collection\n")
    closeDatabase(db)
    return
  }
  cat("Collection created successfully\n")
  
  # 5. 插入向量
  vectors <- list()
  for (i in 0:4) {
    vector <- numeric(dimension)
    for (j in 1:dimension) {
      vector[j] <- i * dimension + (j - 1)
    }
    vectors[[i + 1]] <- vector
  }
  
  for (i in 1:length(vectors)) {
    id <- insertVector(collection, vectors[[i]])
    if (id == 0) {
      cat(paste("Failed to insert vector", i - 1, "\n"))
    } else {
      cat(paste("Inserted vector", i - 1, "with ID:", id, "\n"))
    }
  }
  
  # 6. 搜索向量
  query_vector <- vectors[[1]]
  k <- 3
  search_results <- searchVector(collection, query_vector, k)
  if (length(search_results) == 0) {
    cat("Failed to search\n")
  } else {
    cat(paste("Search results (top", length(search_results$ids), "):\n"))
    for (i in 1:length(search_results$ids)) {
      cat(paste("  ID:", search_results$ids[i], ", Distance:", round(search_results$distances[i], 4), "\n"))
    }
  }
  
  # 7. 列出集合
  collections <- listCollections(db)
  if (length(collections) == 0) {
    cat("Failed to list collections\n")
  } else {
    cat(paste("Collections (", length(collections), "):\n"))
    for (i in 1:length(collections)) {
      cat(paste("  ", collections[i], "\n"))
    }
  }
  
  # 8. 清理资源
  closeDatabase(db)
  
  cat("Demo completed successfully\n")
}

# 运行主函数
main()
