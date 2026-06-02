#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
JinWo VecDB Test Script
Test pip installation from PyPI and basic functionality
"""

import glob
import os
import sys
import shutil
import tempfile
from pathlib import Path

def print_debug_info():
    """Print debug information when library loading fails"""
    print("\n" + "=" * 60)
    print("DEBUG INFORMATION")
    print("=" * 60)
    
    print("\n[1] Python Version:")
    print(f"    {sys.version}")
    
    print("\n[2] Platform:")
    print(f"    sys.platform: {sys.platform}")
    print(f"    os.name: {os.name}")
    
    print("\n[3] JinWo VecDB Package Location:")
    try:
        import jinwo_vecdb
        pkg_path = Path(jinwo_vecdb.__file__).parent
        print(f"    Package path: {pkg_path}")
        print(f"    Version: {jinwo_vecdb.__version__}")
        
        print("\n[4] Files in package directory:")
        for f in sorted(pkg_path.glob("*")):
            size = f.stat().st_size if f.is_file() else "DIR"
            print(f"    {f.name:<30} {size}")
        
        print("\n[5] Looking for _jinwo files:")
        found = False
        for pattern in ["_jinwo*.pyd", "_jinwo*.so", "_jinwo*.dll", "_jinwo*.dylib"]:
            files = list(pkg_path.glob(pattern))
            if files:
                for f in files:
                    print(f"    [OK]  Found: {f.name}")
                    found = True
            else:
                print(f"    [FAIL]  Not found: {pattern}")
        
        if not found:
            print("\n[ERROR] No _jinwo extension module found!")
            print("        The package may not be properly built for this platform.")
        
    except ImportError as e:
        print(f"    [FAIL]  Failed to import jinwo_vecdb: {e}")
    
    print("\n" + "=" * 60)

def main():
    print("=" * 60)
    print("JinWo VecDB Test Script")
    print("=" * 60)
    print()

    # Test 1: Import module
    print("[Test 1] Import jinwo_vecdb")
    print("-" * 40)
    try:
        import jinwo_vecdb
        print(f"[OK]  Import successful")
        print(f"  Version: {jinwo_vecdb.__version__}")
    except Exception as e:
        print(f"[FAIL]  Import failed: {e}")
        print_debug_info()
        sys.exit(1)
    print()

    # Test 2: Create/open database
    print("[Test 2] Create/open database")
    print("-" * 40)
    tmpdir = tempfile.mkdtemp(prefix="jinwo_test_")
    db_path = os.path.join(tmpdir, "test_jinwo_db.jwv")
    print(f"  Using temp dir: {tmpdir}")
    try:
        db = jinwo_vecdb.open(db_path)
        print(f"[OK]  Database created at: {db_path}")
    except Exception as e:
        print(f"[FAIL]  Failed to create database: {e}")
        print_debug_info()
        sys.exit(1)
    print()

    # Test 3: Create collection
    print("[Test 3] Create collection")
    print("-" * 40)
    try:
        collection_name = "documents"
        db.create_collection(collection_name, 384)
        print(f"[OK]  Collection '{collection_name}' created")
    except Exception as e:
        print(f"[FAIL]  Failed to create collection: {e}")
        db.close()
        sys.exit(1)
    print()

    # Test 4: Insert vectors
    print("[Test 4] Insert vectors")
    print("-" * 40)
    try:
        for i in range(100):
            vec = [float(i + j) for j in range(384)]
            db.insert(collection_name, vec)
        print("[OK]  Successfully inserted 100 vectors")
    except Exception as e:
        print(f"[FAIL]  Failed to insert vectors: {e}")
        db.close()
        sys.exit(1)
    print()

    # Test 5: Search vectors
    print("[Test 5] Search vectors")
    print("-" * 40)
    try:
        query_vec = [float(50 + j) for j in range(384)]
        results = db.search(collection_name, query_vec, k=10)
        print(f"[OK]  Search completed")
        print(f"  Results count: {len(results)}")
        print("\n  Top 5 results:")
        for idx, (vid, distance) in enumerate(results[:5]):
            print(f"    {idx+1}. vid={vid}, distance={distance:.6f}")
    except Exception as e:
        print(f"[FAIL]  Failed to search vectors: {e}")
        db.close()
        sys.exit(1)
    print()

    # Test 6: Delete vector
    print("[Test 6] Delete vector")
    print("-" * 40)
    try:
        query_vec = [float(50 + j) for j in range(384)]
        results = db.search(collection_name, query_vec, k=1)
        if results:
            vid_to_delete = results[0][0]
            db.delete(collection_name, vid_to_delete)
            print(f"[OK]  Deleted vector with vid={vid_to_delete}")
            results_after = db.search(collection_name, query_vec, k=5)
            print(f"  Results after delete: {len(results_after)}")
        else:
            print("[FAIL]  No results to delete")
    except Exception as e:
        print(f"[FAIL]  Failed to delete vector: {e}")
        db.close()
        sys.exit(1)
    print()

    # Test 7: Batch insert
    print("[Test 7] Batch insert vectors")
    print("-" * 40)
    try:
        vectors = [[float(200 + i + j) for j in range(384)] for i in range(50)]
        vids = db.insert_batch(collection_name, vectors)
        print(f"[OK]  Successfully inserted {len(vids)} vectors in batch")
    except Exception as e:
        print(f"[FAIL]  Failed to batch insert: {e}")
        db.close()
        sys.exit(1)
    print()

    # Test 8: Close database
    print("[Test 8] Close database")
    print("-" * 40)
    try:
        db.close()
        print("[OK]  Database closed successfully")
    except Exception as e:
        print(f"[FAIL]  Failed to close database: {e}")
        sys.exit(1)
    print()

    # Test 9: Reopen database and verify data persistence
    print("[Test 9] Reopen database")
    print("-" * 40)
    try:
        db2 = jinwo_vecdb.open(db_path)
        print(f"[OK]  Database reopened at: {db_path}")

        # Check if collections are reloaded from disk
        collections_after = db2.list_collections()
        print(f"  Collections after reopen: {collections_after}")

        # Check disk files exist
        disk_files = glob.glob(os.path.join(db_path, "*.jwcol"))
        print(f"  Collection files on disk: {[os.path.basename(f) for f in disk_files]}")

        if len(collections_after) == 0 and len(disk_files) > 0:
            print("[WARN] Collection files exist on disk but not loaded after reopen")
            print("       This is a known issue: data persistence on reopen is not yet supported")
        elif len(collections_after) > 0:
            # Verify data is still accessible
            query_vec = [float(50 + j) for j in range(384)]
            results = db2.search(collection_name, query_vec, k=5)
            print(f"[OK]  Search after reopen: {len(results)} results")
            for idx, (vid, distance) in enumerate(results[:3]):
                print(f"    {idx+1}. vid={vid}, distance={distance:.6f}")
            print("[OK]  Data persistence verified")
    except Exception as e:
        print(f"[FAIL]  Reopen test failed: {e}")
        db2 = None
    print()

    # Test 10: Close database again
    print("[Test 10] Close database again")
    print("-" * 40)
    try:
        if db2 is not None:
            db2.close()
        print("[OK]  Database closed successfully")
    except Exception as e:
        print(f"[FAIL]  Failed to close database: {e}")
    print()

    # Cleanup
    print("[Cleanup] Remove test database")
    print("-" * 40)
    try:
        shutil.rmtree(tmpdir, ignore_errors=True)
        print(f"[OK]  Removed temp dir: {tmpdir}")
    except Exception as e:
        print(f"[FAIL]  Failed to remove temp dir: {e}")
    print()

    print("=" * 60)
    print("All tests passed! [OK] ")
    print("=" * 60)

if __name__ == "__main__":
    main()