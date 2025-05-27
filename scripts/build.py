# coding=utf-8
import shutil
import os
import sys
import argparse

# 获取当前脚本的绝对路径
SCRIPT_PATH = os.path.split(os.path.realpath(__file__))[0]
# 构建目录路径,位于脚本目录的上一级build文件夹
BUILD_DIR_PATH = SCRIPT_PATH + '/../build'


# 清理构建目录
def clear():
    if os.path.exists(BUILD_DIR_PATH):
        shutil.rmtree(BUILD_DIR_PATH)


# Windows平台构建函数
def build_windows(platform='x64', config='Release', args=None):
    # 创建平台特定的构建目录,如 build/x64-Debug
    platform_dir = '%s/%s-%s' % (BUILD_DIR_PATH, platform, config)
    os.makedirs(platform_dir, exist_ok=True)

    # 切换到构建目录
    os.chdir(platform_dir)

    # 生成CMake构建命令
    build_cmd = 'cmake ../.. -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=%s -DCMAKE_GENERATOR_PLATFORM=%s -T v143' % (
        config, platform)

    # 根据命令行参数添加测试和示例的编译选项
    if args.test:
        build_cmd += ' -DBUILD_BURIED_TEST=ON'

    if args.example:
        build_cmd += ' -DBUILD_BURIED_EXAMPLES=ON'

    print("build cmd:" + build_cmd)
    # 执行CMake命令生成项目文件
    ret = os.system(build_cmd)
    if ret != 0:
        print('!!!!!!!!!!!!!!!!!!build fail')
        return False

    # 执行构建命令,使用8个线程并行构建
    build_cmd = 'cmake --build . --config %s --parallel 8' % (config)
    ret = os.system(build_cmd)
    if ret != 0:
        print('build fail!!!!!!!!!!!!!!!!!!!!')
        return False
    return True


def main():
    # 清理并重新创建构建目录
    clear()
    os.makedirs(BUILD_DIR_PATH, exist_ok=True)

    # 创建命令行参数解析器
    parser = argparse.ArgumentParser(description='build windows')
    # 添加test参数,用于控制是否构建单元测试
    parser.add_argument('--test', action='store_true', default=False,
                        help='run unittest')
    # 添加example参数,用于控制是否构建示例
    parser.add_argument('--example', action='store_true', default=False,
                        help='run examples')
    args = parser.parse_args()

    # 执行Windows x64 Debug版本构建
    if not build_windows(platform='x64', config='Debug', args=args):
        exit(1)

if __name__ == '__main__':
    main()