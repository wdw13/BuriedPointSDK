#include <filesystem>

#include "gtest/gtest.h"
#include "src/database/database.h"

// 数据库基本功能测试（插入、查询、删除）
TEST(DbTest, DISABLED_BasicTest) {
  // 数据库文件路径
  std::filesystem::path db_path("hello_world.db");

  // 如果数据库文件已存在，先删除
  if (std::filesystem::exists(db_path)) {
    std::filesystem::remove(db_path);
  }

  // 创建数据库对象
  buried::BuriedDb db(db_path.string());

  // 插入第一条数据
  {
    buried::BuriedDb::Data data{-1, 1, 2,
                                std::vector<char>{'h', 'e', 'l', 'l', 'o'}};
    db.InsertData(data);
  }
  // 查询数据，验证数量
  auto datas = db.QueryData(10);
  EXPECT_EQ(datas.size(), 1);

  // 插入第二条数据
  {
    buried::BuriedDb::Data data{-1, 2, 3,
                                std::vector<char>{'h', 'e', 'l', 'l', 'o'}};
    db.InsertData(data);
  }
  datas = db.QueryData(10);
  EXPECT_EQ(datas.size(), 2);

  // 插入第三条数据
  {
    buried::BuriedDb::Data data{-1, 3, 4,
                                std::vector<char>{'h', 'e', 'l', 'l', 'o'}};
    db.InsertData(data);
  }
  datas = db.QueryData(10);
  EXPECT_EQ(datas.size(), 3);

  // 插入第四条数据
  {
    buried::BuriedDb::Data data{-1, 4, 5,
                                std::vector<char>{'h', 'e', 'l', 'l', 'o'}};
    db.InsertData(data);
  }
  datas = db.QueryData(10);
  EXPECT_EQ(datas.size(), 4);

  // 检查数据按 priority 降序排列
  EXPECT_EQ(datas[0].priority, 4);
  EXPECT_EQ(datas[1].priority, 3);
  EXPECT_EQ(datas[2].priority, 2);
  EXPECT_EQ(datas[3].priority, 1);

  // 删除第一条数据（priority=4），再查询
  db.DeleteData(datas[0]);
  datas = db.QueryData(10);
  EXPECT_EQ(datas.size(), 3);

  // 连续插入100条数据
  for (int i = 0; i < 100; ++i) {
    buried::BuriedDb::Data data{-1, i, i,
                                std::vector<char>{'h', 'e', 'l', 'l', 'o'}};
    db.InsertData(data);
  }

  // 查询前10条数据
  datas = db.QueryData(10);
  EXPECT_EQ(datas.size(), 10);

  // 删除数据库文件，清理环境
  std::filesystem::remove(db_path);
}