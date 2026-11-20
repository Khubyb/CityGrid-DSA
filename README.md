# TGS CityGrid Operations Center — Data Structures Assignment 4 (Project Phase 1)

**Name:** Muhammad Khubaib
**Roll number:** 25L-3026
**Section:** BSE-3B, Fall 2026

## 1. Build and run

Exact build command (run from the project root):

```
g++ -std=c++17 -Wall -Icore -Iindex -Iengine engine/*.cpp -o citygrid
```

Run: `./citygrid <junctions file> <incidents file> <script file>`

Run all tests (after building): `bash tests/run_tests.sh`

Only allowed library features are used (`<iostream>`, `<fstream>`, `<string>`, `<utility>` for
`std::swap`, raw arrays with `new[]/delete[]`). No STL containers, no `<algorithm>`, no smart pointers.

## 2. Project layout

| Folder / file | Contents |
|---|---|
| `core/` | `Junction.h`, `Incident.h` (record structs + priority comparator) |
| `index/` | `AVL.h`, `BST.h`, `HashMap.h`, `MaxHeap.h` (templates, header-only) |
| `engine/` | `CityGrid.h/.cpp` (loading, parsing, synchronisation), `main.cpp` |
| `tests/` | 15 test folders + `run_tests.sh` |

## 3. Responsibility of each class

* **AVL<K,V>** – junction directory. Stores `Junction` values. `insert` goes down recursively; on the way
  back up each node's stored height is updated and the first node with |balance| = 2 is fixed with an
  LL, RR, LR or RL rotation (heights are never recomputed for the whole tree). Also `find`,
  `inorder`, `preorder`. Height of empty = 0, leaf = 1, balance = height(left) − height(right).
* **BST<K,V>** – incident registry (plain, unbalanced). Stores `Incident*` and **owns** the objects.
  Iterative insert/find/remove. `remove` handles leaf, one child and two children (in-order
  successor). Also `inorder` and `preorder`.
* **HashMap<K,V>** – O(1)-average incident lookup (`Incident*` values). Contains exactly the active
  incidents. Used for `INC_FIND`, duplicate checks, and the record lookup in `REQUEST` / `INC_CLOSE`.
* **MaxHeap<T>** – array-based binary max-heap (0-indexed) of `Incident*` for the pending queue:
  `insert` (sift-up), `peek`, `extractMax` (last → root, sift-down), `size`, `removeById`.
* **CityGrid** – the engine. Reads the three files, validates every line, parses commands, prints all
  output, and keeps the four structures synchronized. The four structures never print anything and never
  parse text.

## 4. Hash table: collision method, deletion, resizing

* **Collision method: separate chaining** (my own singly linked list per bucket, new entries are
  added at the head of the list).
* Hash function (fixed): `index = key % capacity`. Initial capacity 11.
* **Deletion:** walk the bucket's list, unlink the node and `delete` it (no tombstones are needed with
  chaining). The `Incident` object is *not* touched.
* **Resizing:** load factor = active entries / capacity. Right after an insertion, if it is
  greater than 0.75 (checked as `count*4 > capacity*3`) the table grows to the smallest prime
  ≥ 2 × old capacity (11 → 23 → 47 → 97 → 197 …). Every node is re-linked into the new bucket array
  with `key % newCapacity` (O(n)). The table never shrinks. Example: with capacity 11 the 9th entry
  makes 9/11 = 0.82 > 0.75, so the table becomes 23 (8/11 = 0.73 does not trigger it).
* `HASH_STATS` prints the current capacity and the number of active entries.

## 5. Priority comparator (in my own words)

Incident A comes **before** incident B in the queue if:
1. A's severity is higher (5 beats 1); otherwise
2. if the severities are equal, A was reported earlier (smaller timestamp); otherwise
3. if the timestamps are also equal, A has the smaller incident ID.

Because IDs are unique, two different incidents are never tied, so the order is completely determined.
The comparator is `higherPriority(const Incident*, const Incident*)` in `core/Incident.h`.

## 6. Record ownership (who allocates, who frees)

* `CityGrid::registerIncident` allocates each `Incident` with **one** `new`. There is exactly one object
  per active incident and it is never copied.
* The **BST owns it**. It is deleted only by `BST::remove` (or the BST destructor at exit).
* The **hash table and the heap only store the same `Incident*`** and never `delete` it.
* Closing (`INC_CLOSE` or `PRIORITY_SERVE`) always follows the same safe order:
  1. if the incident is PENDING it is taken out of the heap (`removeById` for `INC_CLOSE`,
     `extractMax` for `PRIORITY_SERVE`);
  2. the incident ID is copied into a local variable;
  3. it is removed from the hash table;
  4. it is removed from the BST, which finally frees the object.

  So when the object is freed, no other structure still holds its pointer → **no dangling pointers**.
  After closing, the ID behaves like an ID that never existed (and can be reported again).
* In the BST two-children deletion, the incident being closed is deleted first, then the
  successor's **pointer** (not the object) is moved into the node, and only the successor *node* is deleted.
* Junctions are stored by value inside the AVL tree and are never removed. All destructors free
  their nodes/arrays; the program was checked with AddressSanitizer + LeakSanitizer (no errors, no leaks).

## 7. Time complexity of every command

n = items in the structure, h = current height of the BST.

| Command | Structures used | Complexity |
|---|---|---|
| `CITY_ADD`, `CITY_FIND` | AVL | O(log n) |
| `CITY_LIST`, `CITY_PREORDER` | AVL traversal | O(n) |
| `INC_REPORT` | hash (duplicate check) + AVL (junction check) + BST insert + hash insert | O(h + log n) + O(1) average |
| `INC_FIND` | hash table | O(1) average |
| `INC_LIST`, `INC_PREORDER` | BST traversal | O(n) |
| `INC_CLOSE` | hash lookup, heap remove (if pending), BST delete, hash delete | O(n) heap search + O(log n) + O(h) + O(1) average |
| `REQUEST` | hash lookup + heap insert | O(1) average + O(log n) |
| `PRIORITY_NEXT`, `PRIORITY_COUNT` | heap | O(1) |
| `PRIORITY_SERVE` | heap extract-max + hash delete + BST delete | O(log n) + O(1) average + O(h) |
| `HASH_STATS` | hash table | O(1) |
| Hash resize (only when triggered) | hash table | O(n) |

**When does the BST become O(n)?** The BST is not balanced. If incidents are inserted in sorted ID order
(1, 2, 3, …, or descending), every new node goes to the same side and the tree turns into a linked list
with h = n. Then insert, find and remove inside the BST cost O(n) instead of O(log n). Random insertion
order gives h ≈ O(log n) on average.

## 8. Other implementation notes

* Files are opened in the order junctions, incidents, script; if any cannot be opened the program prints
  `FATAL: cannot open <file>` and exits with code 1 before processing starts.
* Line numbers count every physical line (blank and comment lines too); `\r` at the end of a line is removed.
* Integer fields accept only digits 0–9 (1 to 9 digits, leading zeros allowed).
* Diagnostics are checked in the order of Section 6 of the assignment (command → argument count →
  field validity → duplicates → existence → status); the first failure wins and the program continues.

## 9. Tests (15 folders in `tests/`)

Each folder holds `junctions.txt`, `incidents.txt`, `script.txt`, `expected_output.txt`.

| Test folder | Edge case(s) covered |
|---|---|
| `test01_worked_example` | The exact worked example of the assignment (Section 10): RR rotation, ties on severity, cancel, serve, unknown command |
| `test02_avl_rr_ll` | Empty tree preorder; RR rotation (ascending 10,20,30); LL rotation; `CITY_LIST` sorted |
| `test03_avl_lr_rl` | LR rotation; RL rotation |
| `test04_avl_ascending_dup_missing` | Ascending insertion 1..7 (perfectly balanced result); duplicate junction; missing junction; name with spaces from file |
| `test05_bst_delete_all_cases` | Delete leaf, one child, two children (successor is direct child and successor deeper in right subtree), root, last remaining incident; `INC_LIST`/`INC_PREORDER` on empty tree; close of already-closed ID |
| `test06_bst_sorted_report` | Empty-tree `INC_LIST`; sorted insertion (degenerate BST); duplicate incident; missing junction; boundary values (time 0, time 999999999) |
| `test07_load_junction_errors_crlf` | Windows `\r\n` endings, blank/whitespace lines, field trimming, every junction rejection reason, 10-digit / signed / decimal / spaced numbers, leading zeros (`007`), duplicate ID, wrong field count (too few and too many) |
| `test08_load_incident_errors` | Every incident rejection reason in the required order, lowercase keywords, severity/timestamp bounds, duplicate before junction check, junction not found, leading zeros |
| `test09_hash_collision_resize` | Colliding IDs 101/112/123 (all `% 11 = 2`); lookup after deleting a collider; re-reporting a closed ID; resize 11→23 exactly at the 9th entry; resize 23→47; `HASH_STATS` after closes |
| `test10_hash_bst_consistency` | Hash, BST and heap stay consistent after request, serve, close, double close, close then REQUEST, re-report |
| `test11_heap_tie_breaks` | Severity ordering; tie on severity (earlier time first); tie on severity and time (smaller ID first); `PRIORITY_NEXT`/`SERVE` on empty heap |
| `test12_heap_cancel_and_growth` | `INC_CLOSE` on a PENDING incident that is not at the top (and at the top); duplicate `REQUEST`; heap array growth past 16 elements |
| `test13_command_errors` | Unknown command (and case-sensitivity), wrong argument count for every command type, each invalid-field reason, duplicate vs. junction-not-found order, status error, script continues after every error, comments, blank lines, extra spaces and tabs |
| `test14_empty_files` | Empty junctions and incidents files; all list/preorder/priority commands on empty structures; report before the junction exists |
| `test15_single_record` | Single junction and incident through the whole life cycle (report, request, serve, empty again) |
