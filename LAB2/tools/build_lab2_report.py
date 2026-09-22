from copy import deepcopy
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile

from docx import Document
from docx.enum.section import WD_SECTION
from docx.enum.table import WD_CELL_VERTICAL_ALIGNMENT, WD_TABLE_ALIGNMENT
from docx.enum.text import WD_ALIGN_PARAGRAPH, WD_BREAK, WD_LINE_SPACING
from docx.opc.constants import RELATIONSHIP_TYPE as RT
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from docx.shared import Inches, Pt, RGBColor


ROOT = Path(r"D:\02_Academic\大三\机器人小组项目")
TEMPLATE = ROOT / "LAB1" / "LAB1_English_Report_ICRA_TemplateBased_Ubuntu20.04.docx"
OUT = ROOT / "LAB2" / "LAB2_English_Report_IEEE_Template.docx"
IMAGE_DIR = ROOT / "LAB2" / "图片"
RELATIVE_TRAJECTORY_PLOT = ROOT / "LAB2" / "tools" / "relative_trajectory_plane.png"


BLACK = "000000"
DARK_GRAY = "666666"
MID_GRAY = "BFBFBF"
LIGHT_GRAY = "F2F2F2"
PALE_GRAY = "E7E6E6"
REPOSITORY_URL = "https://github.com/lizh-coder/Robotics-Integration-Team-Project-I"


def add_alt_text_to_header_images(path):
    """Give inherited floating header graphics meaningful Word alt text."""
    from lxml import etree

    temp = Path(str(path) + ".tmp")
    ns = {"wp": "http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing"}
    with ZipFile(path, "r") as source, ZipFile(temp, "w", ZIP_DEFLATED) as target:
        for item in source.infolist():
            data = source.read(item.filename)
            if item.filename.startswith("word/header") and item.filename.endswith(".xml"):
                root = etree.fromstring(data)
                changed = False
                for node in root.xpath(".//wp:docPr", namespaces=ns):
                    if not node.get("descr") and not node.get("title"):
                        node.set("descr", "IEEE report template header graphic")
                        node.set("title", "IEEE report header")
                        changed = True
                if changed:
                    data = etree.tostring(root, xml_declaration=True, encoding="UTF-8", standalone="yes")
            target.writestr(item, data)
    temp.replace(path)


def set_cell_shading(cell, fill):
    tc_pr = cell._tc.get_or_add_tcPr()
    shd = tc_pr.find(qn("w:shd"))
    if shd is None:
        shd = OxmlElement("w:shd")
        tc_pr.append(shd)
    shd.set(qn("w:fill"), fill)


def add_hyperlink(paragraph, text, url):
    """Add a black, underlined external hyperlink compatible with Word."""
    relationship_id = paragraph.part.relate_to(url, RT.HYPERLINK, is_external=True)
    hyperlink = OxmlElement("w:hyperlink")
    hyperlink.set(qn("r:id"), relationship_id)
    run = OxmlElement("w:r")
    run_pr = OxmlElement("w:rPr")
    color = OxmlElement("w:color")
    color.set(qn("w:val"), BLACK)
    underline = OxmlElement("w:u")
    underline.set(qn("w:val"), "single")
    font_size = OxmlElement("w:sz")
    font_size.set(qn("w:val"), "17")
    run_pr.extend([color, underline, font_size])
    run.append(run_pr)
    text_node = OxmlElement("w:t")
    text_node.text = text
    run.append(text_node)
    hyperlink.append(run)
    paragraph._p.append(hyperlink)


def set_cell_margins(cell, top=90, start=100, bottom=90, end=100):
    tc = cell._tc
    tc_pr = tc.get_or_add_tcPr()
    tc_mar = tc_pr.first_child_found_in("w:tcMar")
    if tc_mar is None:
        tc_mar = OxmlElement("w:tcMar")
        tc_pr.append(tc_mar)
    for m, val in (("top", top), ("start", start), ("bottom", bottom), ("end", end)):
        node = tc_mar.find(qn(f"w:{m}"))
        if node is None:
            node = OxmlElement(f"w:{m}")
            tc_mar.append(node)
        node.set(qn("w:w"), str(val))
        node.set(qn("w:type"), "dxa")


def set_table_borders(table, top="000000", middle="666666", bottom="000000"):
    tbl_pr = table._tbl.tblPr
    borders = tbl_pr.first_child_found_in("w:tblBorders")
    if borders is None:
        borders = OxmlElement("w:tblBorders")
        tbl_pr.append(borders)
    for edge in ("top", "left", "bottom", "right", "insideH", "insideV"):
        node = borders.find(qn(f"w:{edge}"))
        if node is None:
            node = OxmlElement(f"w:{edge}")
            borders.append(node)
        if edge == "top":
            node.set(qn("w:val"), "single")
            node.set(qn("w:sz"), "12")
            node.set(qn("w:color"), top)
        elif edge == "bottom":
            node.set(qn("w:val"), "single")
            node.set(qn("w:sz"), "12")
            node.set(qn("w:color"), bottom)
        elif edge == "insideH":
            node.set(qn("w:val"), "single")
            node.set(qn("w:sz"), "6")
            node.set(qn("w:color"), middle)
        else:
            node.set(qn("w:val"), "nil")


def set_three_line_table(table):
    set_table_borders(table)
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    table.autofit = True
    # Mark the first row for Word accessibility and repeating-header behavior.
    header_pr = table.rows[0]._tr.get_or_add_trPr()
    header_pr.append(OxmlElement("w:tblHeader"))
    for row_i, row in enumerate(table.rows):
        row._tr.get_or_add_trPr().append(OxmlElement("w:cantSplit"))
        for cell in row.cells:
            cell.vertical_alignment = WD_CELL_VERTICAL_ALIGNMENT.CENTER
            set_cell_margins(cell)
            if row_i == 0:
                set_cell_shading(cell, PALE_GRAY)
            for p in cell.paragraphs:
                p.paragraph_format.space_before = Pt(0)
                p.paragraph_format.space_after = Pt(0)
                p.paragraph_format.line_spacing = 1.0
                for run in p.runs:
                    run.font.name = "Times New Roman"
                    run.font.size = Pt(8.3)
                    run.font.color.rgb = RGBColor.from_string(BLACK)
                    if row_i == 0:
                        run.bold = True


def set_cell_text(cell, text, bold=False, align=WD_ALIGN_PARAGRAPH.LEFT):
    cell.text = ""
    p = cell.paragraphs[0]
    p.alignment = align
    p.paragraph_format.space_before = Pt(0)
    p.paragraph_format.space_after = Pt(0)
    run = p.add_run(text)
    run.font.name = "Times New Roman"
    run.font.size = Pt(8.3)
    run.font.color.rgb = RGBColor.from_string(BLACK)
    run.bold = bold


def set_keep(p, keep_next=False, keep_lines=True):
    p_pr = p._p.get_or_add_pPr()
    if keep_next:
        node = OxmlElement("w:keepNext")
        p_pr.append(node)
    if keep_lines:
        node = OxmlElement("w:keepLines")
        p_pr.append(node)


def add_para(doc, text="", style="Normal", align=None, before=0, after=3, first_indent=None):
    p = doc.add_paragraph(style=style)
    if align is not None:
        p.alignment = align
    p.paragraph_format.space_before = Pt(before)
    p.paragraph_format.space_after = Pt(after)
    p.paragraph_format.line_spacing = 1.0
    if first_indent is not None:
        p.paragraph_format.first_line_indent = Inches(first_indent)
    if text:
        run = p.add_run(text)
        run.font.name = "Times New Roman"
        run.font.color.rgb = RGBColor.from_string(BLACK)
    return p


def add_body(doc, text):
    return add_para(doc, text, style="Normal", after=3, first_indent=0.16)


def add_heading(doc, text, level=1):
    style = "Heading 1" if level == 1 else "Heading 2"
    p = add_para(doc, text.upper() if level == 1 else text, style=style, before=5, after=2)
    set_keep(p, keep_next=True)
    for r in p.runs:
        r.font.name = "Times New Roman"
        r.font.color.rgb = RGBColor.from_string(BLACK)
    return p


def add_code(doc, text):
    p = add_para(doc, text, style="Normal", after=3)
    p.paragraph_format.left_indent = Inches(0.08)
    p.paragraph_format.right_indent = Inches(0.04)
    p.paragraph_format.first_line_indent = Inches(0)
    p.paragraph_format.line_spacing = 1.0
    for r in p.runs:
        r.font.name = "Courier New"
        r.font.size = Pt(7.5)
        r.font.color.rgb = RGBColor.from_string(DARK_GRAY)
    return p


def add_equation(doc, text):
    p = add_para(doc, text, style="Equation" if "Equation" in [s.name for s in doc.styles] else "Normal", align=WD_ALIGN_PARAGRAPH.CENTER, before=1, after=3)
    p.paragraph_format.first_line_indent = Inches(0)
    for r in p.runs:
        r.font.name = "Cambria Math"
        r.font.size = Pt(8.5)
        r.font.color.rgb = RGBColor.from_string(BLACK)
    return p


def add_table(doc, title, headers, rows, widths=None):
    p = add_para(doc, title, style="Table Title", before=3, after=1)
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    set_keep(p, keep_next=True)
    table = doc.add_table(rows=1, cols=len(headers))
    for j, h in enumerate(headers):
        set_cell_text(table.rows[0].cells[j], h, bold=True, align=WD_ALIGN_PARAGRAPH.CENTER)
    for data in rows:
        cells = table.add_row().cells
        for j, value in enumerate(data):
            set_cell_text(cells[j], str(value), align=WD_ALIGN_PARAGRAPH.CENTER if j == 0 else WD_ALIGN_PARAGRAPH.LEFT)
    if widths:
        for row in table.rows:
            for j, width in enumerate(widths):
                row.cells[j].width = Inches(width)
    set_three_line_table(table)
    add_para(doc, "", after=1)
    return table


def add_figure_placeholder(doc, caption, instruction, height=0.72):
    p = add_para(doc, caption, style="Figure Caption", before=3, after=1)
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    set_keep(p, keep_next=True)
    table = doc.add_table(rows=1, cols=1)
    table.rows[0]._tr.get_or_add_trPr().append(OxmlElement("w:tblHeader"))
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    table.autofit = False
    cell = table.cell(0, 0)
    cell.width = Inches(3.25)
    cell.height = Inches(height)
    set_cell_shading(cell, LIGHT_GRAY)
    set_cell_margins(cell, top=130, start=100, bottom=130, end=100)
    set_table_borders(table, top=MID_GRAY, middle=MID_GRAY, bottom=MID_GRAY)
    cell.text = ""
    p2 = cell.paragraphs[0]
    p2.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p2.paragraph_format.space_before = Pt(0)
    p2.paragraph_format.space_after = Pt(0)
    p2.paragraph_format.line_spacing = 1.0
    r = p2.add_run("SCREENSHOT PLACEHOLDER\n" + instruction)
    r.font.name = "Arial"
    r.font.size = Pt(8)
    r.font.color.rgb = RGBColor.from_string(DARK_GRAY)
    add_para(doc, "", after=1)


def add_figure(doc, caption, image_paths, widths):
    """Insert evidence as inline images with an adjacent IEEE-style caption."""
    for path, width in zip(image_paths, widths):
        if not path.exists():
            raise FileNotFoundError(f"Figure image is missing: {path}")
        p = doc.add_paragraph()
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        p.paragraph_format.space_before = Pt(2)
        p.paragraph_format.space_after = Pt(1)
        picture = p.add_run().add_picture(str(path), width=Inches(width))
        picture._inline.docPr.set("descr", caption)
        picture._inline.docPr.set("title", caption.split(".", 1)[0])
        set_keep(p, keep_next=True)
    p = add_para(doc, caption, style="Figure Caption", before=0, after=3)
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    set_keep(p)


def add_bullet(doc, text):
    p = add_para(doc, text, style="Normal", after=2)
    p.paragraph_format.left_indent = Inches(0.16)
    p.paragraph_format.first_line_indent = Inches(-0.11)
    return p


def clear_body(doc):
    body = doc._element.body
    sect_pr = body.sectPr
    for child in list(body):
        if child is not sect_pr:
            body.remove(child)


def normalize_document(doc):
    for style_name in ["Normal", "Heading 1", "Heading 2", "Caption", "Figure Caption", "Table Title", "Abstract", "IndexTerms", "Authors"]:
        if style_name not in [s.name for s in doc.styles]:
            continue
        st = doc.styles[style_name]
        st.font.name = "Times New Roman"
        st.font.color.rgb = RGBColor.from_string(BLACK)
    doc.core_properties.title = "LAB 2 ROS Installation and Use"
    doc.core_properties.subject = "MIT VNAV Lab 2 experiment report"
    doc.core_properties.author = "Li Zehao"


def build():
    doc = Document(str(TEMPLATE))
    clear_body(doc)
    normalize_document(doc)

    # IEEE-style title block inherited from the LAB1 template.
    title_style = "Title" if "Title" in [s.name for s in doc.styles] else "Normal"
    p = add_para(doc, "LAB 2: ROS INSTALLATION AND USE", style=title_style, align=WD_ALIGN_PARAGRAPH.CENTER, after=2)
    for r in p.runs:
        r.font.name = "Times New Roman"
        r.font.size = Pt(16)
        r.font.bold = True
        r.font.color.rgb = RGBColor.from_string(BLACK)
    p = add_para(doc, "Li Zehao", style="Authors", align=WD_ALIGN_PARAGRAPH.CENTER, after=1)
    for r in p.runs:
        r.font.name = "Times New Roman"
        r.font.size = Pt(10)
        r.font.color.rgb = RGBColor.from_string(BLACK)
    p = add_para(doc, "Student ID: 24020036039", style="Authors", align=WD_ALIGN_PARAGRAPH.CENTER, after=1)
    for r in p.runs:
        r.font.name = "Times New Roman"
        r.font.size = Pt(8.5)
        r.font.italic = True
        r.font.color.rgb = RGBColor.from_string(DARK_GRAY)
    p = doc.add_paragraph(style="Authors")
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.space_before = Pt(0)
    p.paragraph_format.space_after = Pt(4)
    lead = p.add_run("Repository: ")
    lead.font.name = "Times New Roman"
    lead.font.size = Pt(8.5)
    lead.font.color.rgb = RGBColor.from_string(DARK_GRAY)
    add_hyperlink(p, "GitHub repository: lizh-coder/Robotics-Integration-Team-Project-I", REPOSITORY_URL)

    p = add_para(doc, "Abstract - This report documents the installation and use of ROS 1 Noetic for the MIT VNAV Lab 2 two-drone exercise. The work reuses the supplied two_drones_pkg skeleton, builds a catkin workspace under Ubuntu 20.04 on WSL2, publishes the world-to-drone transforms, queries relative transforms with tf2, and visualizes the trajectories in RViz. The dynamic implementation was verified at 50 Hz, and the required homogeneous-transform and quaternion derivations are included. Runtime screenshots remain marked as placeholders for insertion from the student's own terminal and RViz session.", style="Abstract", after=3)
    for r in p.runs:
        r.font.name = "Times New Roman"
        r.font.size = Pt(8.5)
        r.font.color.rgb = RGBColor.from_string(BLACK)
    p = add_para(doc, "Keywords - ROS 1, ROS Noetic, WSL2, tf2, RViz, catkin, homogeneous transforms, quadrotor", style="IndexTerms", after=5)
    for r in p.runs:
        r.font.name = "Times New Roman"
        r.font.size = Pt(8.5)
        r.font.color.rgb = RGBColor.from_string(BLACK)

    add_heading(doc, "1. Experiment Objectives and Reuse", 1)
    add_body(doc, "The objective of LAB2 is to use ROS 1 to organize a package, compile a catkin workspace, publish coordinate transforms, query transforms with tf2, and visualize a two-drone scene. The experiment follows the MIT VNAV Lab 2 exercises. The existing LAB1 repository already contained the official two_drones_pkg skeleton, so a copy was made under LAB2/lab2/two_drones_pkg. This preserved the earlier submission while allowing the LAB2 implementation to be completed independently.")
    add_body(doc, "The completed work includes the 50 Hz world-to-av1 and world-to-av2 transform publisher, the lookupTransform call used by the trajectory publisher, explicit geometry_msgs, tf2_geometry_msgs, and visualization_msgs dependencies, and the ROS launch and RViz configuration. The course-provided quadrotor.dae is referenced by the two mesh markers. On this WSLg host, RViz must be launched with software OpenGL rendering because the D3D12 backend loads the mesh but renders it blank.")
    add_table(doc, "TABLE I. EXPERIMENT SCOPE", ["Item", "Implementation status"], [
        ("ROS installation", "ROS Noetic verified in Ubuntu 20.04 WSL2"),
        ("Workspace", "catkin workspace /root/vnav_ws built successfully"),
        ("Package", "two_drones_pkg found with rospack"),
        ("Transform publisher", "Implemented and verified at 50 Hz"),
        ("Transform listener", "lookupTransform implemented for three trails"),
        ("Screenshots", "Evidence captured and embedded in this report"),
        ("Student information", "Li Zehao; Student ID: 24020036039"),
    ], [1.25, 2.0])

    add_heading(doc, "2. Experimental Environment", 1)
    add_body(doc, "The experiment was performed in Ubuntu 20.04.3 LTS running under WSL2 on Windows. ROS 1 Noetic, catkin, tf2, RViz, and the required message packages are installed. The source package is stored on the D: drive and copied into the Ubuntu workspace at /root/vnav_ws/src/two_drones_pkg.")
    add_table(doc, "TABLE II. SOFTWARE ENVIRONMENT", ["Component", "Verified configuration"], [
        ("Operating system", "Ubuntu 20.04.3 LTS on WSL2"),
        ("ROS distribution", "Noetic"),
        ("Build system", "catkin build"),
        ("Visualization", "RViz 1.14.26"),
        ("Workspace", "/root/vnav_ws"),
        ("Package path", "/root/vnav_ws/src/two_drones_pkg"),
        ("Rendering setting", "LIBGL_ALWAYS_SOFTWARE=true for RViz on this host"),
    ], [1.25, 2.0])
    add_code(doc, "wsl -d Ubuntu-20.04\nsource /opt/ros/noetic/setup.bash\nsource /root/vnav_ws/devel/setup.bash\nrosversion -d\nrospack find two_drones_pkg")
    add_figure(doc, "Fig. 1. ROS and package environment verification.", [IMAGE_DIR / "1.png"], [3.25])

    add_heading(doc, "3. Deliverable 1: Nodes, Topics, and Launch", 1)
    add_body(doc, "The launch file two_drones.launch has one static argument. With static:=true, two static_transform_publisher nodes provide the initial frame arrangement. With the default static:=false, frames_publisher_node publishes the dynamic transforms. In both modes plots_publisher_node publishes a MarkerArray on /visuals and RViz subscribes to /visuals, /tf, and /tf_static.")
    add_table(doc, "TABLE III. ROS NODES AND CONNECTIONS", ["Node", "Role", "Relevant interface"], [
        ("/frames_publisher_node", "Dynamic transform publisher", "/tf, 50 Hz per transform"),
        ("/av1broadcaster", "Static world to av1 transform", "/tf_static, static mode"),
        ("/av2broadcaster", "Static world to av2 transform", "/tf_static, static mode"),
        ("/plots_publisher_node", "MarkerArray and trajectory publisher", "publishes /visuals; listens through tf2"),
        ("/rviz", "Visualization", "subscribes to /visuals and TF"),
        ("/rosout", "ROS logging", "system logging topic"),
    ], [1.25, 1.1, 0.9])
    add_body(doc, "The static mode is first used to verify package discovery, frame names, and the RViz configuration. The dynamic mode is then used for the actual motion and trajectory experiment.")
    add_code(doc, "# Terminal 1\nsource /opt/ros/noetic/setup.bash\nroscore\n\n# Terminal 2: static verification\nsource /opt/ros/noetic/setup.bash\nsource /root/vnav_ws/devel/setup.bash\nroslaunch two_drones_pkg two_drones.launch static:=true")
    add_figure(doc, "Fig. 2. Static launch and RViz verification.", [IMAGE_DIR / "2.png"], [3.25])
    add_code(doc, "rosnode list\nrostopic list\nrosnode info /plots_publisher_node\nrostopic info /visuals")
    add_figure(doc, "Fig. 3. Node and topic inspection.", [IMAGE_DIR / "3.png"], [2.7])

    add_heading(doc, "4. Deliverable 2: Publishing Coordinate Transforms", 1)
    add_body(doc, "The dynamic publisher creates a 50 Hz timer. If t denotes the elapsed time since startup, the two drone origins are prescribed by the exercise as follows:")
    add_equation(doc, "o₁ʷ(t) = [ cos(t),  sin(t),  0 ]ᵀ")
    add_equation(doc, "o₂ʷ(t) = [ sin(t),  0,  cos(2t) ]ᵀ")
    add_body(doc, "For AV1, the yaw is t. The body y axis is therefore tangent to [cos(t), sin(t), 0], because its tangent direction is [-sin(t), cos(t), 0]. The z axis remains parallel to the world z axis. AV2 uses the identity rotation because only its position is needed for the requested trajectory derivations.")
    add_code(doc, "roslaunch two_drones_pkg two_drones.launch\nrosrun tf tf_echo world av1\nrosrun tf tf_echo world av2\nrostopic hz /tf")
    add_figure(doc, "Fig. 4. Dynamic motion in RViz.", [IMAGE_DIR / "4.png"], [3.25])
    add_figure(doc, "Fig. 5. Dynamic TF evidence: changing world-to-av1 transform and /tf publication rate.", [IMAGE_DIR / "5-1.png", IMAGE_DIR / "5-2.png"], [2.6, 2.6])

    add_heading(doc, "5. Deliverable 3: Querying Relative Transforms", 1)
    add_body(doc, "The trajectory publisher receives a reference frame and a destination frame for each trail. The latest available transform is obtained with:")
    add_code(doc, "transform = parent->tf_buffer.lookupTransform(\n    ref_frame, dest_frame, ros::Time(0));")
    add_body(doc, "The first trail queries world to av1 and is shown in blue. The second queries world to av2 and is shown in orange. The third queries av1 to av2 and is drawn as an orange dashed curve. When RViz Fixed Frame is changed from world to av1, AV1 becomes stationary and the relative AV2 curve is shown in the moving AV1 frame.")
    add_table(doc, "TABLE IV. TRAJECTORY MARKERS", ["Namespace", "Lookup call", "Expected appearance"], [
        ("Trail av1-world", "lookupTransform(world, av1, 0)", "Blue solid circular trail"),
        ("Trail av2-world", "lookupTransform(world, av2, 0)", "Orange solid x-z trail"),
        ("Trail av2-av1", "lookupTransform(av1, av2, 0)", "Orange dashed relative trail"),
    ], [1.15, 1.25, 0.85])
    add_code(doc, "# In RViz, change Global Options -> Fixed Frame to av1\nrosrun tf tf_echo av1 av2")
    add_figure(doc, "Fig. 6. AV2 relative to AV1 with Fixed Frame set to av1.", [IMAGE_DIR / "6.png"], [3.25])

    add_heading(doc, "6. Deliverable 4: Homogeneous Transforms and Trajectory Derivations", 1)
    add_heading(doc, "6.1 AV2 in the world frame", 2)
    add_body(doc, "Let x = sin(t). Then cos(2t) = 1 - 2 sin²(t), so the AV2 path satisfies:")
    add_equation(doc, "y = 0,    z = 1 - 2x²")
    add_body(doc, "The path is a parabolic arc in the world x-z plane.")
    add_heading(doc, "6.2 Relative homogeneous transform", 2)
    add_body(doc, "Let ʷT₁ and ʷT₂ map coordinates from AV1 and AV2 to the world frame. With AV1 yaw equal to t and AV2 rotation equal to I:")
    add_equation(doc, "¹T₂ = (ʷT₁)⁻¹ ʷT₂,    o₂¹ = Rz(-t) (o₂ʷ - o₁ʷ)")
    add_equation(doc, "o₂¹(t) = [ -1 + 0.5 sin(2t),  -0.5 + 0.5 cos(2t),  cos(2t) ]ᵀ")
    add_heading(doc, "6.3 Plane and ellipse", 2)
    add_body(doc, "The relative coordinates satisfy z = 2y + 1, so the trajectory lies on the plane z - 2y = 1. Choose the center p = [-1, -0.5, 0]ᵀ and the right-handed orthonormal basis:")
    add_equation(doc, "xₚ = [1,0,0]ᵀ,  yₚ = [0,1/√5,2/√5]ᵀ,  zₚ = [0,-2/√5,1/√5]ᵀ")
    add_equation(doc, "o₂ᵖ(t) = [ 0.5 sin(2t),  (√5/2) cos(2t),  0 ]ᵀ")
    add_body(doc, "Therefore the trajectory is an ellipse in the plane z - 2y = 1. Its semiaxes are 0.5 and √5/2, with the longer semiaxis along yₚ.")
    add_figure(doc, "Fig. 7. Relative AV2 trajectory in the centered p frame; the ellipse lies on z - 2y = 1.", [RELATIVE_TRAJECTORY_PLOT], [3.25])

    add_heading(doc, "7. Deliverable 5: Quaternion Linear Maps", 1)
    add_body(doc, "Use q = [q₁,q₂,q₃,q₄]ᵀ with the first three entries as the imaginary part and q₄ as the scalar part. Quaternion multiplication can be written in two linear forms:")
    add_equation(doc, "qₐ ⊗ qᵦ = Ω₁(qₐ) qᵦ = Ω₂(qᵦ) qₐ")
    add_code(doc, "Ω₁(q) = [ q₄  -q₃   q₂   q₁ ]       Ω₂(q) = [ q₄   q₃  -q₂   q₁ ]\n"
                  "        [ q₃   q₄  -q₁   q₂ ]               [ -q₃  q₄   q₁   q₂ ]\n"
                  "        [ -q₂  q₁   q₄   q₃ ]               [ q₂  -q₁   q₄   q₃ ]\n"
                  "        [ -q₁ -q₂  -q₃   q₄ ]               [ -q₁ -q₂  -q₃   q₄ ]")
    add_body(doc, "For a unit quaternion q, both left and right multiplication preserve the quaternion norm because quaternion norms are multiplicative. Hence ||Ω₁(q)x|| = ||x|| and ||Ω₂(q)x|| = ||x|| for every x in R⁴, which proves Ωᵢ(q)ᵀΩᵢ(q) = Ωᵢ(q)Ωᵢ(q)ᵀ = I₄. The fourth column of both displayed matrices is q, so Ωᵢ(q)e₄ = q. Multiplying by Ωᵢ(q)ᵀ gives Ωᵢ(q)ᵀq = e₄, where e₄ = [0,0,0,1]ᵀ.")
    add_body(doc, "For arbitrary x, y, and z, Ω₁(x)Ω₂(y)z = x ⊗ (z ⊗ y) while Ω₂(y)Ω₁(x)z = (x ⊗ z) ⊗ y. Associativity proves Ω₁(x)Ω₂(y) = Ω₂(y)Ω₁(x). Since Ω₂(y)ᵀ = Ω₂(conj(y)), replacing y by conj(y) proves Ω₁(x)Ω₂(y)ᵀ = Ω₂(y)ᵀΩ₁(x).")

    add_heading(doc, "8. Optional Deliverable 6: Intrinsic and Extrinsic Rotations", 1)
    add_body(doc, "For column vectors and active rotations, extrinsic rotations about fixed world axes multiply on the left, giving R_ext = R₂R₁R₀. Intrinsic rotations about the current body axes multiply on the right. If the intrinsic sequence is applied in reverse order, the accumulated expression is also R₂R₁R₀, so both descriptions produce the same final orientation. The required numerical substitution is:")
    add_equation(doc, "R₀ = Rx(90°),    R₁ = Ry(180°),    R₂ = Rx(-30°)")
    add_equation(doc, "R₂R₁R₀ = [ -1   0          0       ;   0  -1/2  -√3/2  ;   0  -√3/2   1/2 ]")
    add_body(doc, "Thus the extrinsic sequence and the reversed intrinsic sequence have exactly the same final orientation. This completes the optional deliverable without requiring an additional ROS screenshot.")

    add_heading(doc, "9. Complete Operation and Screenshot Procedure", 1)
    add_body(doc, "The following sequence produces the evidence required for the final report. Run each command in a separate Ubuntu terminal unless the command is explicitly marked as a continuation in the same terminal.")
    steps = [
        "1. Enter Ubuntu from PowerShell with: wsl -d Ubuntu-20.04.",
        "2. In Terminal 1 run source /opt/ros/noetic/setup.bash and roscore. Capture the terminal only if the instructor requests ROS master evidence.",
        "3. In Terminal 2 run source /opt/ros/noetic/setup.bash, source /root/vnav_ws/devel/setup.bash, rosversion -d, which roscore, which rviz, and rospack find two_drones_pkg. Capture Fig. 1.",
        "4. Run roslaunch two_drones_pkg two_drones.launch static:=true. Wait for RViz. Confirm Fixed Frame = world, MarkerArray status OK, and AVs checked. Capture Fig. 2.",
        "5. In Terminal 3 run rosnode list, rostopic list, rosnode info /plots_publisher_node, and rostopic info /visuals. Capture Fig. 3.",
        "6. Stop only the launch with Ctrl+C. Keep roscore running.",
        "7. Run roslaunch two_drones_pkg two_drones.launch. Wait 5 to 10 seconds so the three trails become visible. Capture Fig. 4.",
        "8. In Terminal 3 run rosrun tf tf_echo world av1 and rostopic hz /tf. Wait until translation values change and the frequency stabilizes. Capture Fig. 5.",
        "9. In RViz change Global Options -> Fixed Frame from world to av1. Keep MarkerArray and all four namespaces enabled. Run rosrun tf tf_echo av1 av2 in Terminal 3. Capture Fig. 6.",
        "10. The captured screenshots are embedded in Figs. 1 through 6. Fig. 7 is the analytic relative-trajectory plot generated from the derived expression.",
        "11. Before submission, confirm that the document title, author name, student ID, figures, and all mathematical deliverables are present.",
    ]
    for s in steps:
        add_bullet(doc, s)
    add_table(doc, "TABLE V. FINAL SUBMISSION CHECKLIST", ["Evidence", "Status before student insertion"], [
        ("ROS Noetic and package path", "Verified in Fig. 1"),
        ("Static nodes and topics", "Verified in Figs. 2 and 3"),
        ("Dynamic AV1 and AV2 motion", "Verified in Fig. 4"),
        ("Changing TF values and frequency", "Verified in Fig. 5"),
        ("Relative AV2-to-AV1 view", "Verified in Fig. 6"),
        ("Math derivations", "Deliverables 4-6 completed"),
        ("Author information", "Li Zehao; 24020036039"),
    ], [1.65, 1.6])

    add_heading(doc, "10. Conclusion", 1)
    add_body(doc, "LAB2 established a working ROS 1 Noetic environment and used it to publish, query, and visualize the two-drone coordinate frames. The implementation satisfies the required node, topic, launch, transform, and tf2 lookup functions. The analytic results show that the AV2 trajectory is a parabola in the world frame and an ellipse on the plane z - 2y = 1 when expressed relative to AV1. The remaining report work is limited to inserting the student's screenshots and completing the identifying information and any instructor-required explicit matrix expansion.")

    add_heading(doc, "References", 1)
    add_body(doc, "[1] MIT VNAV Lab 2 Exercises, https://vnav.mit.edu/labs_2023/lab2/exercises.html")
    add_body(doc, "[2] MIT-SPARK, VNAV-labs, https://github.com/MIT-SPARK/VNAV-labs")
    add_body(doc, "[3] ROS Noetic documentation and tf2 tutorials, https://wiki.ros.org/tf2")
    add_body(doc, "[4] LAB2 course slides, LAB2/Lab2.pdf")

    doc.save(str(OUT))
    add_alt_text_to_header_images(OUT)
    print(OUT)


if __name__ == "__main__":
    build()
