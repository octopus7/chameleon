import { writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const outDir = dirname(fileURLToPath(import.meta.url));
const objPath = join(outDir, "SM_HiderMetaball_Body.obj");
const mtlPath = join(outDir, "SM_HiderMetaball_Body.mtl");

const threshold = 0.62;
const scaleToCm = 58.0;

const blobs = [
  { c: [0.0, 0.0, 2.66], r: [0.42, 0.42, 0.44], s: 1.06, bone: "Head" },
  { c: [0.0, 0.0, 2.22], r: [0.20, 0.24, 0.28], s: 0.82, bone: "Neck" },
  { c: [0.0, 0.0, 1.72], r: [0.43, 0.42, 0.58], s: 1.00, bone: "UpperTorso" },
  { c: [0.02, 0.0, 1.18], r: [0.42, 0.36, 0.50], s: 0.90, bone: "LowerTorso" },
  { c: [0.0, 0.0, 0.78], r: [0.42, 0.44, 0.30], s: 0.82, bone: "Root" },

  { c: [0.0, -0.48, 1.88], r: [0.22, 0.28, 0.23], s: 0.76, bone: "LeftUpperArm" },
  { c: [0.0, -0.76, 1.88], r: [0.18, 0.34, 0.18], s: 0.92, bone: "LeftUpperArm" },
  { c: [0.0, -1.08, 1.88], r: [0.18, 0.32, 0.18], s: 0.92, bone: "LeftLowerArm" },
  { c: [0.0, -1.39, 1.88], r: [0.17, 0.34, 0.17], s: 0.90, bone: "LeftLowerArm" },
  { c: [0.0, -1.64, 1.88], r: [0.17, 0.22, 0.17], s: 0.78, bone: "LeftLowerArm" },
  { c: [0.0, 0.48, 1.88], r: [0.22, 0.28, 0.23], s: 0.76, bone: "RightUpperArm" },
  { c: [0.0, 0.76, 1.88], r: [0.18, 0.34, 0.18], s: 0.92, bone: "RightUpperArm" },
  { c: [0.0, 1.08, 1.88], r: [0.18, 0.32, 0.18], s: 0.92, bone: "RightLowerArm" },
  { c: [0.0, 1.39, 1.88], r: [0.17, 0.34, 0.17], s: 0.90, bone: "RightLowerArm" },
  { c: [0.0, 1.64, 1.88], r: [0.17, 0.22, 0.17], s: 0.78, bone: "RightLowerArm" },

  { c: [0.0, -0.24, 0.48], r: [0.24, 0.22, 0.46], s: 0.92, bone: "LeftUpperLeg" },
  { c: [0.0, -0.30, 0.12], r: [0.21, 0.19, 0.38], s: 0.88, bone: "LeftLowerLeg" },
  { c: [0.13, -0.31, -0.05], r: [0.34, 0.17, 0.14], s: 0.72, bone: "LeftLowerLeg" },
  { c: [0.0, 0.24, 0.48], r: [0.24, 0.22, 0.46], s: 0.92, bone: "RightUpperLeg" },
  { c: [0.0, 0.30, 0.12], r: [0.21, 0.19, 0.38], s: 0.88, bone: "RightLowerLeg" },
  { c: [0.13, 0.31, -0.05], r: [0.34, 0.17, 0.14], s: 0.72, bone: "RightLowerLeg" },
];

const bounds = {
  min: [-0.80, -1.88, -0.22],
  max: [0.92, 1.88, 3.10],
};

const steps = [40, 96, 78];
const cubeCorners = [
  [0, 0, 0],
  [1, 0, 0],
  [1, 1, 0],
  [0, 1, 0],
  [0, 0, 1],
  [1, 0, 1],
  [1, 1, 1],
  [0, 1, 1],
];

const tetrahedra = [
  [0, 5, 1, 6],
  [0, 1, 2, 6],
  [0, 2, 3, 6],
  [0, 3, 7, 6],
  [0, 7, 4, 6],
  [0, 4, 5, 6],
];

function valueAt(p) {
  let value = 0.0;

  for (const blob of blobs) {
    const dx = (p[0] - blob.c[0]) / blob.r[0];
    const dy = (p[1] - blob.c[1]) / blob.r[1];
    const dz = (p[2] - blob.c[2]) / blob.r[2];
    value += blob.s * Math.exp(-(dx * dx + dy * dy + dz * dz));
  }

  return value;
}

function gradientAt(p) {
  const g = [0.0, 0.0, 0.0];

  for (const blob of blobs) {
    const x = p[0] - blob.c[0];
    const y = p[1] - blob.c[1];
    const z = p[2] - blob.c[2];
    const dx = x / blob.r[0];
    const dy = y / blob.r[1];
    const dz = z / blob.r[2];
    const e = blob.s * Math.exp(-(dx * dx + dy * dy + dz * dz));

    g[0] += e * -2.0 * x / (blob.r[0] * blob.r[0]);
    g[1] += e * -2.0 * y / (blob.r[1] * blob.r[1]);
    g[2] += e * -2.0 * z / (blob.r[2] * blob.r[2]);
  }

  return normalize([-g[0], -g[1], -g[2]]);
}

function normalize(v) {
  const length = Math.hypot(v[0], v[1], v[2]);

  if (length < 1e-8) {
    return [0.0, 0.0, 1.0];
  }

  return [v[0] / length, v[1] / length, v[2] / length];
}

function subtract(a, b) {
  return [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
}

function cross(a, b) {
  return [
    a[1] * b[2] - a[2] * b[1],
    a[2] * b[0] - a[0] * b[2],
    a[0] * b[1] - a[1] * b[0],
  ];
}

function dot(a, b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

function isLeftLimbBone(bone) {
  return bone === "LeftUpperArm"
    || bone === "LeftLowerArm"
    || bone === "LeftUpperLeg"
    || bone === "LeftLowerLeg";
}

function isRightLimbBone(bone) {
  return bone === "RightUpperArm"
    || bone === "RightLowerArm"
    || bone === "RightUpperLeg"
    || bone === "RightLowerLeg";
}

function isArmBone(bone) {
  return bone === "LeftUpperArm"
    || bone === "LeftLowerArm"
    || bone === "RightUpperArm"
    || bone === "RightLowerArm";
}

function isLegBone(bone) {
  return bone === "LeftUpperLeg"
    || bone === "LeftLowerLeg"
    || bone === "RightUpperLeg"
    || bone === "RightLowerLeg";
}

function smoothStep01(value) {
  const t = Math.min(Math.max(value, 0.0), 1.0);
  return t * t * (3.0 - 2.0 * t);
}

function sameSideGate(bone, p) {
  const side = p[1];
  const absSide = Math.abs(side);

  if ((isLeftLimbBone(bone) && side > -0.015) || (isRightLimbBone(bone) && side < 0.015)) {
    return 0.0;
  }

  if (isArmBone(bone)) {
    return smoothStep01((absSide - 0.25) / 0.20);
  }

  if (isLegBone(bone)) {
    return smoothStep01((absSide - 0.08) / 0.16);
  }

  return 1.0;
}

function dominantBoneAt(p) {
  let dominantBone = "Root";
  let bestContribution = 0.0;

  for (const blob of blobs) {
    const gate = sameSideGate(blob.bone, p);
    if (gate <= 1e-8) {
      continue;
    }

    const dx = (p[0] - blob.c[0]) / blob.r[0];
    const dy = (p[1] - blob.c[1]) / blob.r[1];
    const dz = (p[2] - blob.c[2]) / blob.r[2];
    const contribution = blob.s * Math.exp(-(dx * dx + dy * dy + dz * dz)) * gate;
    if (contribution > bestContribution) {
      bestContribution = contribution;
      dominantBone = blob.bone;
    }
  }

  return dominantBone;
}

const uvIslands = [
  "HeadFront",
  "HeadBack",
  "HeadLeft",
  "HeadRight",
  "TorsoFront",
  "TorsoBack",
  "TorsoLeft",
  "TorsoRight",
  "LeftArmFront",
  "LeftArmBack",
  "LeftArmTop",
  "LeftArmBottom",
  "RightArmFront",
  "RightArmBack",
  "RightArmTop",
  "RightArmBottom",
  "LeftLegFront",
  "LeftLegBack",
  "LeftLegOuter",
  "LeftLegInner",
  "RightLegFront",
  "RightLegBack",
  "RightLegOuter",
  "RightLegInner",
];

function selectFourWayBodyIsland(normal, front, back, left, right) {
  if (Math.abs(normal[0]) >= Math.abs(normal[1])) {
    return normal[0] >= 0.0 ? front : back;
  }

  return normal[1] < 0.0 ? left : right;
}

function selectUvIsland(p, normal) {
  const bone = dominantBoneAt(p);

  if (bone === "Head" || (bone === "Neck" && p[2] >= 2.16)) {
    return selectFourWayBodyIsland(normal, "HeadFront", "HeadBack", "HeadLeft", "HeadRight");
  }

  if (isArmBone(bone)) {
    const leftArm = isLeftLimbBone(bone);
    if (Math.abs(normal[0]) >= Math.abs(normal[2])) {
      if (leftArm) {
        return normal[0] >= 0.0 ? "LeftArmFront" : "LeftArmBack";
      }

      return normal[0] >= 0.0 ? "RightArmFront" : "RightArmBack";
    }

    if (leftArm) {
      return normal[2] >= 0.0 ? "LeftArmTop" : "LeftArmBottom";
    }

    return normal[2] >= 0.0 ? "RightArmTop" : "RightArmBottom";
  }

  if (isLegBone(bone)) {
    const leftLeg = isLeftLimbBone(bone);
    if (Math.abs(normal[0]) >= Math.abs(normal[1])) {
      if (leftLeg) {
        return normal[0] >= 0.0 ? "LeftLegFront" : "LeftLegBack";
      }

      return normal[0] >= 0.0 ? "RightLegFront" : "RightLegBack";
    }

    if (leftLeg) {
      return normal[1] < 0.0 ? "LeftLegOuter" : "LeftLegInner";
    }

    return normal[1] > 0.0 ? "RightLegOuter" : "RightLegInner";
  }

  return selectFourWayBodyIsland(normal, "TorsoFront", "TorsoBack", "TorsoLeft", "TorsoRight");
}

function normalizeRange(value, min, max) {
  return Math.min(Math.max((value - min) / Math.max(max - min, 0.001), 0.0), 1.0);
}

function localUvForIsland(island, p) {
  switch (island) {
    case "HeadFront":
    case "HeadBack":
      return [normalizeRange(p[1], -0.48, 0.48), normalizeRange(p[2], 2.14, 3.08)];

    case "HeadLeft":
    case "HeadRight":
      return [normalizeRange(p[0], -0.48, 0.48), normalizeRange(p[2], 2.14, 3.08)];

    case "TorsoFront":
    case "TorsoBack":
      return [normalizeRange(p[1], -0.58, 0.58), normalizeRange(p[2], 0.42, 2.36)];

    case "TorsoLeft":
    case "TorsoRight":
      return [normalizeRange(p[0], -0.56, 0.58), normalizeRange(p[2], 0.42, 2.36)];

    case "LeftArmFront":
    case "LeftArmBack":
      return [normalizeRange(p[1], -1.76, -0.40), normalizeRange(p[2], 1.54, 2.20)];

    case "RightArmFront":
    case "RightArmBack":
      return [normalizeRange(p[1], 0.40, 1.76), normalizeRange(p[2], 1.54, 2.20)];

    case "LeftArmTop":
    case "LeftArmBottom":
      return [normalizeRange(p[1], -1.76, -0.40), normalizeRange(p[0], -0.30, 0.30)];

    case "RightArmTop":
    case "RightArmBottom":
      return [normalizeRange(p[1], 0.40, 1.76), normalizeRange(p[0], -0.30, 0.30)];

    case "LeftLegFront":
    case "LeftLegBack":
      return [normalizeRange(p[1], -0.54, -0.06), normalizeRange(p[2], -0.16, 0.88)];

    case "RightLegFront":
    case "RightLegBack":
      return [normalizeRange(p[1], 0.06, 0.54), normalizeRange(p[2], -0.16, 0.88)];

    case "LeftLegOuter":
    case "LeftLegInner":
    case "RightLegOuter":
    case "RightLegInner":
      return [normalizeRange(p[0], -0.34, 0.56), normalizeRange(p[2], -0.16, 0.88)];

    default:
      return [0.5, 0.5];
  }
}

function packUvIsland(island, uv) {
  const columns = 6;
  const rows = 4;
  const marginU = 0.010;
  const marginV = 0.014;
  const islandIndex = Math.max(uvIslands.indexOf(island), 0);
  const column = islandIndex % columns;
  const row = Math.floor(islandIndex / columns);
  const cellWidth = 1.0 / columns;
  const cellHeight = 1.0 / rows;
  const innerWidth = cellWidth - marginU * 2.0;
  const innerHeight = cellHeight - marginV * 2.0;

  return [
    column * cellWidth + marginU + uv[0] * innerWidth,
    row * cellHeight + marginV + uv[1] * innerHeight,
  ];
}

function unwrappedUv(island, p) {
  return packUvIsland(island, localUvForIsland(island, p));
}

function average(points) {
  return [
    (points[0][0] + points[1][0] + points[2][0]) / 3.0,
    (points[0][1] + points[1][1] + points[2][1]) / 3.0,
    (points[0][2] + points[1][2] + points[2][2]) / 3.0,
  ];
}

function interpolate(a, b) {
  const t = (threshold - a.value) / (b.value - a.value);

  return [
    a.p[0] + (b.p[0] - a.p[0]) * t,
    a.p[1] + (b.p[1] - a.p[1]) * t,
    a.p[2] + (b.p[2] - a.p[2]) * t,
  ];
}

function orientTriangle(points) {
  const rightHandedNormal = cross(subtract(points[1], points[0]), subtract(points[2], points[0]));
  const center = [
    (points[0][0] + points[1][0] + points[2][0]) / 3.0,
    (points[0][1] + points[1][1] + points[2][1]) / 3.0,
    (points[0][2] + points[1][2] + points[2][2]) / 3.0,
  ];
  const outward = gradientAt(center);

  // The OBJ source is authored in UE's left-handed X-forward/Y-right/Z-up space.
  // Keep vertex normals outward, but wind faces for UE's counter-clockwise front-face convention.
  if (dot(rightHandedNormal, outward) > 0.0) {
    return [points[0], points[2], points[1]];
  }

  return points;
}

function emitTetra(vertices, faces) {
  const inside = vertices.filter((v) => v.value >= threshold);
  const outside = vertices.filter((v) => v.value < threshold);

  if (inside.length === 0 || inside.length === 4) {
    return;
  }

  if (inside.length === 1 || inside.length === 3) {
    const pivot = inside.length === 1 ? inside[0] : outside[0];
    const others = inside.length === 1 ? outside : inside;

    faces.push(
      orientTriangle([
        interpolate(pivot, others[0]),
        interpolate(pivot, others[1]),
        interpolate(pivot, others[2]),
      ]),
    );
    return;
  }

  const a = inside[0];
  const b = inside[1];
  const c = outside[0];
  const d = outside[1];
  const ac = interpolate(a, c);
  const ad = interpolate(a, d);
  const bc = interpolate(b, c);
  const bd = interpolate(b, d);

  faces.push(orientTriangle([ac, bc, bd]));
  faces.push(orientTriangle([ac, bd, ad]));
}

function pointAt(ix, iy, iz) {
  return [
    bounds.min[0] + ((bounds.max[0] - bounds.min[0]) * ix) / steps[0],
    bounds.min[1] + ((bounds.max[1] - bounds.min[1]) * iy) / steps[1],
    bounds.min[2] + ((bounds.max[2] - bounds.min[2]) * iz) / steps[2],
  ];
}

function buildFaces() {
  const faces = [];

  for (let iz = 0; iz < steps[2]; iz += 1) {
    for (let iy = 0; iy < steps[1]; iy += 1) {
      for (let ix = 0; ix < steps[0]; ix += 1) {
        const cube = cubeCorners.map(([ox, oy, oz]) => {
          const p = pointAt(ix + ox, iy + oy, iz + oz);

          return { p, value: valueAt(p) };
        });

        for (const tetra of tetrahedra) {
          emitTetra(
            tetra.map((cornerIndex) => cube[cornerIndex]),
            faces,
          );
        }
      }
    }
  }

  return faces;
}

function vertexKey(p) {
  return `${p[0].toFixed(5)},${p[1].toFixed(5)},${p[2].toFixed(5)}`;
}

function exportObj(faces) {
  let minZ = Infinity;

  for (const face of faces) {
    for (const p of face) {
      minZ = Math.min(minZ, p[2]);
    }
  }

  const vertices = [];
  const normals = [];
  const uvs = [];
  const indicesByKey = new Map();
  const faceIndices = [];

  for (const face of faces) {
    const indices = [];
    const island = selectUvIsland(average(face), gradientAt(average(face)));

    for (const p of face) {
      const shifted = [p[0], p[1], p[2] - minZ];
      const key = `${vertexKey(shifted)},${island}`;
      let index = indicesByKey.get(key);

      if (index === undefined) {
        vertices.push(shifted);
        normals.push(gradientAt(p));
        uvs.push(unwrappedUv(island, p));
        index = vertices.length;
        indicesByKey.set(key, index);
      }

      indices.push(index);
    }

    faceIndices.push(indices);
  }

  const lines = [
    "# Generated by generate-hider-metaball-body.mjs",
    "# Coordinate system: X forward, Y right, Z up. Units: centimeters.",
    "mtllib SM_HiderMetaball_Body.mtl",
    "o SM_HiderMetaball_Body",
    "usemtl PaintableBody",
  ];

  for (const v of vertices) {
    lines.push(
      `v ${(v[0] * scaleToCm).toFixed(4)} ${(v[1] * scaleToCm).toFixed(4)} ${(v[2] * scaleToCm).toFixed(4)}`,
    );
  }

  for (const uv of uvs) {
    lines.push(`vt ${uv[0].toFixed(6)} ${uv[1].toFixed(6)}`);
  }

  for (const n of normals) {
    lines.push(`vn ${n[0].toFixed(6)} ${n[1].toFixed(6)} ${n[2].toFixed(6)}`);
  }

  for (const face of faceIndices) {
    lines.push(`f ${face.map((i) => `${i}/${i}/${i}`).join(" ")}`);
  }

  writeFileSync(objPath, `${lines.join("\n")}\n`, "utf8");
  writeFileSync(
    mtlPath,
    [
      "# Paintable off-white placeholder material for UE import.",
      "newmtl PaintableBody",
      "Ka 0.8400 0.8200 0.7600",
      "Kd 0.8400 0.8200 0.7600",
      "Ks 0.0400 0.0400 0.0400",
      "Ns 12.0000",
      "d 1.0000",
      "illum 2",
      "",
    ].join("\n"),
    "utf8",
  );

  return { vertexCount: vertices.length, faceCount: faceIndices.length };
}

const faces = buildFaces();
const stats = exportObj(faces);

console.log(`Wrote ${objPath}`);
console.log(`Wrote ${mtlPath}`);
console.log(`Vertices: ${stats.vertexCount}`);
console.log(`Faces: ${stats.faceCount}`);
