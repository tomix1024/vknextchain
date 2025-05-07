import re

vulkan_core_filename = 'Vulkan-Headers/include/vulkan/vulkan_core.h'

# Regex expressions
match_struct = re.compile(r'^typedef struct (Vk\w+) {$', re.MULTILINE)
match_enum_open = re.compile(r'^typedef enum VkStructureType {$', re.MULTILINE)
match_enum_close = re.compile(r'^} VkStructureType;$', re.MULTILINE)
match_comment_multiline = re.compile(r'/\*.*\*/', re.MULTILINE | re.DOTALL)
match_comment_singleline = re.compile(r'//.*$', re.MULTILINE)
match_struct_type = re.compile(r'VK_STRUCTURE_TYPE_\w+')
match_header_version = re.compile(r'^#define VK_HEADER_VERSION ([0-9]+)$', re.MULTILINE)

def remove_comments(prog, text):
    pos = 0
    keep_text = []
    while pos < len(text):
        result = prog.search(text, pos)
        if not result:
            break
        keep_text.append(text[pos:result.start()])
        pos = result.end()
    if pos < len(text):
        keep_text.append(text[pos:])
    return ''.join(keep_text)


# Read header and remove comments
with open(vulkan_core_filename, 'rt') as f:
    vulkan_core_text = f.read()
vulkan_core_text = remove_comments(match_comment_multiline, vulkan_core_text)
vulkan_core_text = remove_comments(match_comment_singleline, vulkan_core_text)


# Extract all struct names
struct_names = dict()
for result in match_struct.finditer(vulkan_core_text):
    struct_name = result[1]
    struct_names[struct_name.lower().removeprefix('vk')] = struct_name


# Find VkStructureType enum
result_open = match_enum_open.search(vulkan_core_text)
result_close = match_enum_close.search(vulkan_core_text, result_open.end())
assert(result_open and result_close)
vulkan_struct_types_text = vulkan_core_text[result_open.end():result_close.start()]
vulkan_struct_types_lines = vulkan_struct_types_text.split('\n')


output_lines = []

# Remember the source header version
result_version = match_header_version.search(vulkan_core_text)
output_lines.append(f'#define VKNEXTCHAIN_HEADER_VERSION {result_version[1]}\n')
output_lines.append('#define _VKNEXTCHAIN_STR_HELPER(x) #x\n')
output_lines.append('#define _VKNEXTCHAIN_STR(x) _VKNEXTCHAIN_STR_HELPER(x)\n')
output_lines.append('static_assert(VKNEXTCHAIN_HEADER_VERSION == VK_HEADER_VERSION, "vk_next_chain built for header version " _VKNEXTCHAIN_STR(VKNEXTCHAIN_HEADER_VERSION) " but found " _VKNEXTCHAIN_STR(VK_HEADER_VERSION));\n')
output_lines.append('#undef _VKNEXTCHAIN_STR\n')
output_lines.append('#undef _VKNEXTCHAIN_STR_HELPER\n')


# Process VK_STRUCTURE_TYPE_* definitions
for line in vulkan_struct_types_lines:
    if line.startswith('#'):
        # preprocessor directives are used to mask beta features, for which the actual structures are not defined in the header!
        #continue
        output_lines.append(line + '\n')
    result = match_struct_type.search(line)
    if result:
        struct_type = result[0]
        struct_key = struct_type.lower().removeprefix('vk_structure_type').replace('_', '')

        # if the underlying structure is not defined, we are not interested.
        if struct_key in struct_names:
            struct_name = struct_names[struct_key]
            output_lines.append(f'template<> struct StructureType<{struct_name}> : public std::integral_constant<VkStructureType, {struct_type}> {{}};\n')
            # Avoid double-definitions if there are two identical definitions...
            del struct_names[struct_key]

# Write generated output
with open('structuretype-gen.inl', 'wt') as f:
    f.writelines(output_lines)
