import UIKit

class iOSDemo: UIViewController {
    private let outputLabel = UILabel()
    private var outputText = ""
    
    override func viewDidLoad() {
        super.viewDidLoad()
        setupUI()
        runDemo()
    }
    
    private func setupUI() {
        view.backgroundColor = .white
        
        outputLabel.textColor = .black
        outputLabel.font = UIFont.monospacedSystemFont(ofSize: 14, weight: .regular)
        outputLabel.numberOfLines = 0
        outputLabel.textAlignment = .left
        
        let scrollView = UIScrollView()
        scrollView.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(scrollView)
        
        scrollView.addSubview(outputLabel)
        outputLabel.translatesAutoresizingMaskIntoConstraints = false
        
        NSLayoutConstraint.activate([
            scrollView.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor),
            scrollView.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 20),
            scrollView.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -20),
            scrollView.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor),
            
            outputLabel.topAnchor.constraint(equalTo: scrollView.topAnchor),
            outputLabel.leadingAnchor.constraint(equalTo: scrollView.leadingAnchor),
            outputLabel.trailingAnchor.constraint(equalTo: scrollView.trailingAnchor),
            outputLabel.bottomAnchor.constraint(equalTo: scrollView.bottomAnchor),
            outputLabel.widthAnchor.constraint(equalTo: scrollView.widthAnchor)
        ])
    }
    
    private func runDemo() {
        appendOutput("========================================")
        appendOutput("  JinWo VecDB iOS 演示程序")
        appendOutput("========================================")
        appendOutput("")
        
        do {
            let documentsDirectory = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0]
            let dbPath = documentsDirectory + "/vecdb"
            let db = try VecDB(path: dbPath, create: true)
            appendOutput("数据库打开成功")
            
            let version = VecDB.getVersion()
            appendOutput("版本: \(version)")
            appendOutput("")
            
            appendOutput("创建集合...")
            let collection = try db.createCollection(name: "test", dimension: 128)
            appendOutput("集合创建成功: test")
            appendOutput("")
            
            appendOutput("插入10个向量...")
            for i in 0..<10 {
                let vector = generateRandomVector(dimension: 128)
                let id = try collection.insert(vector: vector)
                appendOutput("插入向量 \(i) 成功，ID: \(id)")
            }
            appendOutput("插入完成")
            appendOutput("")
            
            appendOutput("搜索相似向量...")
            let query = generateRandomVector(dimension: 128)
            let results = try collection.search(query: query, k: 5)
            if !results.isEmpty {
                appendOutput("搜索结果 (前\(results.count)个):")
                for (index, result) in results.enumerated() {
                    appendOutput("ID: \(result.id), 距离: \(String(format: "%.4f", result.distance))")
                }
            } else {
                appendOutput("搜索失败，无结果")
            }
            
            appendOutput("")
            appendOutput("列出所有集合...")
            let collections = try db.listCollections()
            appendOutput("共有 \(collections.count) 个集合:")
            for name in collections {
                appendOutput("  \(name)")
            }
            
            appendOutput("")
            appendOutput("演示完成!")
        } catch let error as VecDBError {
            appendOutput("错误: \(error.localizedDescription)")
        } catch {
            appendOutput("未知错误: \(error.localizedDescription)")
        }
    }
    
    private func generateRandomVector(dimension: Int) -> [Float] {
        return (0..<dimension).map { _ in Float.random(in: -1.0...1.0) }
    }
    
    private func appendOutput(_ text: String) {
        outputText += text + "\n"
        updateOutput()
    }
    
    private func updateOutput() {
        DispatchQueue.main.async {
            self.outputLabel.text = self.outputText
        }
    }
}

class VecDB {
    private let dbPointer: UnsafeMutableRawPointer
    
    init(path: String, create: Bool = true) throws {
        var ptr: UnsafeMutableRawPointer?
        let result = jw_vecdb_open(&ptr, path, create)
        guard result == 0, let pointer = ptr else {
            throw VecDBError(code: result)
        }
        dbPointer = pointer
    }
    
    deinit {
        jw_vecdb_close(dbPointer)
    }
    
    static func getVersion() -> String {
        let versionPtr = jw_version()
        return String(cString: versionPtr)
    }
    
    func createCollection(name: String, dimension: Int) throws -> Collection {
        var colPtr: UnsafeMutableRawPointer?
        let result = jw_collection_create(&colPtr, dbPointer, name, dimension)
        guard result == 0, let pointer = colPtr else {
            throw VecDBError(code: result)
        }
        return Collection(pointer: pointer)
    }
    
    func listCollections() throws -> [String] {
        var names: UnsafeMutablePointer<UnsafeMutablePointer<Int8>?>?
        var count: size_t = 0
        let result = jw_collection_list(dbPointer, &names, &count)
        guard result == 0, let collectionNames = names else {
            throw VecDBError(code: result)
        }
        
        defer {
            for i in 0..<count {
                if let name = collectionNames[Int(i)] {
                    free(name)
                }
            }
            free(collectionNames)
        }
        
        var resultArray: [String] = []
        for i in 0..<count {
            if let name = collectionNames[Int(i)] {
                let string = String(cString: name)
                resultArray.append(string)
            }
        }
        return resultArray
    }
}

class Collection {
    private let collectionPointer: UnsafeMutableRawPointer
    
    init(pointer: UnsafeMutableRawPointer) {
        collectionPointer = pointer
    }
    
    deinit {
        jw_collection_close(collectionPointer)
    }
    
    func insert(vector: [Float]) throws -> UInt64 {
        var id: UInt64 = 0
        let result = vector.withUnsafeBufferPointer { buffer in
            jw_collection_insert(collectionPointer, buffer.baseAddress, &id)
        }
        guard result == 0 else {
            throw VecDBError(code: result)
        }
        return id
    }
    
    func search(query: [Float], k: Int) throws -> [(id: UInt64, distance: Float)] {
        var results: UnsafeMutablePointer<jw_search_result_t>?
        var count: size_t = 0
        let result = query.withUnsafeBufferPointer { buffer in
            jw_collection_search(collectionPointer, buffer.baseAddress, size_t(k), &results, &count)
        }
        guard result == 0, let searchResults = results else {
            throw VecDBError(code: result)
        }
        
        defer {
            free(searchResults)
        }
        
        var resultArray: [(id: UInt64, distance: Float)] = []
        for i in 0..<count {
            let item = searchResults[Int(i)]
            resultArray.append((id: item.id, distance: item.distance))
        }
        return resultArray
    }
}

enum VecDBError: Error {
    case code(Int)
    
    var localizedDescription: String {
        switch self {
        case .code(let code):
            switch code {
            case 0: return "成功"
            case -1: return "无效参数"
            case -2: return "内存不足"
            case -3: return "文件系统错误"
            case -4: return "集合已存在"
            case -5: return "集合不存在"
            case -6: return "向量不存在"
            case -7: return "索引创建失败"
            case -8: return "维度不匹配"
            case -9: return "内部错误"
            default: return "未知错误"
            }
        }
    }
}

private extension VecDB {
    @_silgen_name("jw_vecdb_open")
    static func jw_vecdb_open(_ db: inout UnsafeMutableRawPointer?, _ path: String, _ create: Bool) -> Int
    
    @_silgen_name("jw_vecdb_close")
    static func jw_vecdb_close(_ db: UnsafeMutableRawPointer) -> Int
    
    @_silgen_name("jw_version")
    static func jw_version() -> UnsafePointer<CChar>
    
    @_silgen_name("jw_collection_create")
    static func jw_collection_create(_ collection: inout UnsafeMutableRawPointer?, _ db: UnsafeMutableRawPointer, _ name: String, _ dimension: Int) -> Int
    
    @_silgen_name("jw_collection_close")
    static func jw_collection_close(_ collection: UnsafeMutableRawPointer) -> Int
    
    @_silgen_name("jw_collection_list")
    static func jw_collection_list(_ db: UnsafeMutableRawPointer, _ names: inout UnsafeMutablePointer<UnsafeMutablePointer<Int8>?>, _ count: inout size_t) -> Int
    
    @_silgen_name("jw_collection_insert")
    static func jw_collection_insert(_ collection: UnsafeMutableRawPointer, _ vector: UnsafePointer<Float>, _ id: inout UInt64) -> Int
    
    @_silgen_name("jw_collection_search")
    static func jw_collection_search(_ collection: UnsafeMutableRawPointer, _ query: UnsafePointer<Float>, _ k: size_t, _ results: inout UnsafeMutablePointer<jw_search_result_t>?, _ count: inout size_t) -> Int
}

private struct jw_search_result_t {
    var id: UInt64
    var distance: Float
}
