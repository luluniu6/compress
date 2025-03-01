#include <iostream>
#include <vector>
#include <queue>
#include <cstring>
#include <unordered_map>
#include <bitset>

struct node
{
	int weight, parent, lchild, rchild;
	int index;
	bool operator>(const node &n) const
	{
		return weight > n.weight;
	}
};

FILE *fp;												 // 文件指针
int frequency[256] = {0};								 // 统计频率
int byte_count = 0;										 // 记录原二进制文件字节数，便于译码
std::vector<node> h(513);								 // 哈夫曼树
std::unordered_map<unsigned char, std::string> code_map; // 记录每个字节及其编码
char filename[100];
long long bit_sum = 0; // 记录编码的总位数

void test1();				// 测试函数
void test2();				// 测试函数
void file_read();			// 读入文件
void count_frequency();		// 统计频率
void Init_huffman();		// 初始化哈夫曼树
void buildHFM();			// 构造哈夫曼树
void coder();				// 编码
void decoder();				// 译码
void save_coding_to_file(); // 保存编码结果到文件

int main()
{
	file_read();
	count_frequency();
	Init_huffman();
	// test1();
	buildHFM();
	// test2();
	coder();
	save_coding_to_file();
	std::cout << "编码完成" << std::endl;
	decoder();
	std::cout << "译码完成" << std::endl;
	return 0;
}
void test1()
{
	for (int i = 0; i < 256; i++)
	{
		printf("%d的权值是:%d\n", i, h[i].weight);
	}
}
void test2()
{
	for (int i = 0; i < 513; i++)
	{
		printf("%d的父节点是:%d,权值是%d,左孩子是:%d,右孩子是:%d\n", i, h[i].parent, h[i].weight, h[i].lchild, h[i].rchild);
	}
}
void file_read()
{
	printf("请输入文件名称：");
	scanf("%s", filename);
	fp = fopen(filename, "rb");
	if (!fp)
	{
		perror("文件读入失败");
	}
}

void count_frequency()
{
	unsigned char byte;
	while (fread(&byte, sizeof(unsigned char), 1, fp))
	{
		if(byte == 13) continue; // 过滤掉换行符
		byte_count++;
		frequency[byte]++;
	}
}

void Init_huffman()
{
	for (int i = 0; i < 256; i++)
	{
		h[i].weight = frequency[i];
		h[i].index = i;
		h[i].parent = h[i].lchild = h[i].rchild = 0;
	}
	for (int i = 256; i < 513; i++)
	{
		h[i].weight = 0;
		h[i].index = i;
	}
}

void buildHFM()
{

	std::priority_queue<node, std::vector<node>, std::greater<node>> pq;

	// 初始化优先队列，将所有频率不为0的节点入队
	for (int i = 0; i < 256; i++)
	{
		if (h[i].weight > 0)
		{
			h[i].parent = h[i].lchild = h[i].rchild = -1; // 初始化parent, lchild, rchild
			pq.push(h[i]);
		}
	}

	int index = 256; // 哈夫曼树新节点的起始索引

	// 构造哈夫曼树
	while (pq.size() > 1)
	{
		node left = pq.top();
		int left_index = left.index;
		pq.pop();
		node right = pq.top();
		int right_index = right.index;
		pq.pop();

		// 创建新节点
		h[index].weight = left.weight + right.weight;
		h[index].lchild = left_index;
		h[index].rchild = right_index;
		h[index].parent = index + 1;

		// 更新左右孩子的父节点
		h[left_index].parent = index;
		h[right_index].parent = index;

		pq.push(h[index]); // 将新节点入队
		index++;
	}
	h[--index].parent = -1;

	// 检查是否成功构建哈夫曼树
	if (index <= 256)
	{
		fprintf(stderr, "错误：哈夫曼树构建失败\n");
		exit(EXIT_FAILURE);
	}
}

// 编码
void coder()
{
	// 清空已有的编码映射
	code_map.clear();

	// 遍历所有的叶子节点（即index < 256的节点）
	for (int i = 0; i < 256; i++)
	{
		if (h[i].weight > 0) // 如果该字节出现过
		{
			std::string code;
			int current = i;

			// 从当前节点一直向上走，直到到达根节点（父节点为-1）
			while (h[current].parent != -1)
			{
				// 如果是左子树，则编码为 '0'，否则为 '1'
				if (h[h[current].parent].lchild == current)
				{
					code = '0' + code;
				}
				else
				{
					code = '1' + code;
				}
				current = h[current].parent; // 移动到父节点
			}

			// 将当前字节和它的哈夫曼编码存入映射表
			code_map[h[i].index] = code;
		}
	}

	// 打印每个字节及其哈夫曼编码
	for (int i = 0; i < 256; i++)
	{
		if (h[i].weight > 0)
		{
			std::cout << "序号" << i << " 字节 " << std::bitset<8>(i) << " 的哈夫曼编码是: " << code_map[h[i].index].c_str() << std::endl;
		}
	}
}


// 将编码存入文件（使用位运算进行优化）
void save_coding_to_file()
{
	// 打开文件"code.bin"以写入模式（以二进制模式写入）
	FILE *output_file = fopen("code.bin", "wb");
	if (!output_file)
	{
		perror("无法创建code.bin文件");
		exit(EXIT_FAILURE);
	}

	// 将文件指针fp复位到文件开头
	fseek(fp, 0, SEEK_SET);

	unsigned char byte;		  // 存储从原文件读取的字节
	unsigned char buffer = 0; // 缓存字节，用于按位存储
	int bit_count = 0;		  // 缓存中已使用的位数

	// 按照字节读入文件fp并利用code_map输出编码到output_file中
	while (fread(&byte, sizeof(unsigned char), 1, fp))
	{
		// 获取当前字节的哈夫曼编码
		const std::string &code = code_map[byte];
		for (char bit : code)
		{
			// 按位将编码写入缓存
			buffer = (buffer << 1) | (bit - '0'); // 左移并添加新位
			bit_count++;
			bit_sum++;
			// 当缓存满8位时，写入文件
			if (bit_count == 8)
			{
				fwrite(&buffer, sizeof(unsigned char), 1, output_file);
				buffer = 0;	   // 清空缓存
				bit_count = 0; // 重置位数计数
			}
		}
	}

	// 处理最后剩余未满8位的缓存
	if (bit_count > 0)
	{
		buffer = buffer << (8 - bit_count); // 左移填充高位为0
		fwrite(&buffer, sizeof(unsigned char), 1, output_file);
	}

	// 关闭文件
	fclose(output_file);
	std::cout << "编码已保存至code.bin" << std::endl;
}
// 译码函数
void decoder()
{
	// 打开二进制编码文件"code.bin"以读入模式
	FILE *input_file = fopen("code.bin", "rb");
	if (!input_file)
	{
		perror("无法打开code.bin文件");
		exit(EXIT_FAILURE);
	}

	// 打开输出文件"decode.txt"以写入模式
	FILE *output_file = fopen("decode.txt", "w");
	if (!output_file)
	{
		perror("无法创建decode.txt文件");
		fclose(input_file);
		exit(EXIT_FAILURE);
	}

	unsigned char buffer;
	int root = 512 - 1; // 哈夫曼树的根节点
	while (root--)
	{
		if (h[root].parent == -1)
			break;
	}
	int current = root;

	// 按字节从文件读取
	while (fread(&buffer, sizeof(unsigned char), 1, input_file) && bit_sum)
	{
		for (int i = 7; i >= 0; i--)
		{
			if (bit_sum == 0)
				break;
			// 逐位解析（从高位到低位）
			int bit = (buffer >> i) & 1;
			bit_sum--;
			// 根据位值移动到左右子节点
			if (bit == 0)
				current = h[current].lchild;
			else
				current = h[current].rchild;

			// 如果到达叶子节点，输出对应字符
			if (h[current].lchild == -1 && h[current].rchild == -1)
			{
				fputc(current, output_file); // 写入字符
				current = root;				 // 返回根节点
			}
		}
	}

	// 关闭文件
	fclose(input_file);
	fclose(output_file);
	std::cout << "译码结果已保存至decode.txt" << std::endl;
}
